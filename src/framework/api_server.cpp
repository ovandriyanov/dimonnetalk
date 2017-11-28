/*
 *  framework/api_server.cpp
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 27/11/2017.
 *
 */

#include <boost/beast.hpp>
#include <boost/log/trivial.hpp>

#include "framework/api_server.h"
#include "util/asio.h"

namespace framework {

api_server_t::api_server_t(boost::asio::io_service& io_service,
                           const api_server_config_t& api_server_config)
    : service_t{io_service}
    , api_server_config_{api_server_config}
    , port_{0}
{
}

void api_server_t::call_api(std::string api_token, std::string call, nlohmann::json request, cb_t callback)
{
    coroutine_->requests.emplace_back(request_t{std::move(api_token), std::move(call), std::move(request), std::move(callback)});
    if(coroutine_->requests.size() == 1)
        coroutine_->resume();
}

void api_server_t::reload()
{
    if(coroutine_) {
        if(api_server_config_.host == host_ && api_server_config_.port == port_)
            return;
    }

    std::list<request_t> unhandled_requests;
    if(coroutine_) {
        unhandled_requests = std::move(coroutine_->requests);
        stop(*coroutine_);
    }

    coroutine_ = std::make_shared<coroutine_t>(*this, std::move(unhandled_requests));
    coroutine_->resume();
}

void api_server_t::stop(std::function<void()> cb)
{
    if(coroutine_) stop(*coroutine_);
    io_service_.post(std::move(cb));
}

void api_server_t::stop(coroutine_t& coro)
{
    if(coro.requests.empty()) {
        coro.resume();
    } else {
        coro.ssl_stream.next_layer().cancel();
        coro.server_connector.cancel();
        coro.stop = true;
    }
}

api_server_t::coroutine_t::coroutine_t(api_server_t& api_server, std::list<request_t> requests)
    : api_server{api_server}
    , ssl_context{boost::asio::ssl::context::sslv23_client}
    , ssl_stream{api_server.io_service_, ssl_context}
    , server_connector{ssl_stream}
    , requests{std::move(requests)}
    , stop{false}
    , resume{[&](coro_t::push_type& yield)
{
    yield();

    while(true) {
        if(requests.empty()) yield();
        if(stop) return;

        assert(requests.size());
        auto req = std::move(requests.front());
        std::pair<std::exception_ptr, nlohmann::json> api_call_res;

        while(true) {
            if(!ssl_stream.next_layer().is_open()) {
                auto connect_res = server_connector.connect(api_server.host_, api_server.port_, yield, resume);
                if(auto* ep = boost::get<std::exception_ptr>(&connect_res)) {
                    api_call_res.first = *ep;
                } else {
                    api_call_res = do_api_call(req.call, req.api_token, req.request, yield, resume);
                    if(stop) return;
                }
                break;
            } else {
                api_call_res = do_api_call(req.call, req.api_token, req.request, yield, resume);
                if(stop) return;
                if(!api_call_res.first) break;

                // Server probably closed previous connection due to timeout
                // Reconnect and retry API call
                ssl_stream.next_layer().close();
            }
        }

        req.callback(api_call_res.first, api_call_res.second);
        requests.pop_front();
    }
}}
{
}

std::pair<std::exception_ptr, nlohmann::json>
api_server_t::coroutine_t::do_api_call(const std::string& call, const std::string& api_token, const nlohmann::json& request_json,
                                       coro_t::push_type& yield, coro_t::pull_type& resume)
{
    using namespace boost::beast;
    http::request<http::string_body> request;
    request.version(11);
    request.method(http::verb::post);
    request.target("/bot" + api_token + '/' + call);
    request.set(http::field::host, api_server.host_);
    request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    request.body() = request_json.dump();

    auto result = util::async_write(make_shared(ssl_stream), request, yield, resume);
    if(stop) return {};
    if(auto ec = std::get<0>(result))
        return {std::make_exception_ptr(system_error{ec, "Cannot write HTTP request to API server: " + ec.message()}), {}};
    BOOST_LOG_TRIVIAL(trace) << "Wrote HTTP request: \n" << request;

    http::response<http::string_body> response;
    flat_buffer buffer;
    result = util::async_read(make_shared(ssl_stream), buffer, response, yield, resume);
    if(stop) return {};
    if(auto ec = std::get<0>(result))
        return {std::make_exception_ptr(system_error{ec, "Cannot read HTTP response from API server: " + ec.message()}), {}};
    BOOST_LOG_TRIVIAL(trace) << "Got HTTP response: \n" << response;

    auto http_error = [](const std::string& what) {
        return std::runtime_error{"Cannot process HTTP response: " + what};
    };

    if(response.result() != http::status::ok) {
        auto ex = http_error("Server returned HTTP status " + std::to_string(static_cast<int>(response.result())));
        return {std::make_exception_ptr(ex), {}};
    }

    auto content_type = response["Content-Type"];
    if(content_type != "application/json") {
        auto ex = http_error("Invalid content type: " + std::string{content_type});
        return {std::make_exception_ptr(ex), {}};
    }

    try {
        auto response_json = nlohmann::json::parse(response.body());
        if(response_json.at("ok") != true)
            throw http_error("Field 'ok' is not 'True'");
        return {nullptr, response_json.at("result")};
    } catch(const nlohmann::json::exception& ex) {
        return {std::current_exception(), {}};
    }
}

} // namespace framework
