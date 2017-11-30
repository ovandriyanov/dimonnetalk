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
    , server_connector_{io_service}
    , port_{0}
{
}

void api_server_t::call_api(request_t request)
{
    requests_.emplace_back(std::move(request));
    if(resume_ && requests_.size() == 1)
        (*resume_)();
}

void api_server_t::reload()
{
    if(resume_) {
        if(api_server_config_.host == host_ && api_server_config_.port == port_)
            return;
    }

    host_ = api_server_config_.host;
    port_ = api_server_config_.port;

    std::list<request_t> unhandled_requests = std::move(requests_);
    if(resume_)
        stop();

    resume_ = std::make_unique<util::push_coro_t>(
        [&](coro_t::pull_type& yield)
    {
        while(true) {
            if(requests_.empty()) yield();

            assert(requests_.size());
            auto req = std::move(requests_.front());
            std::pair<std::exception_ptr, nlohmann::json> api_call_res;

            while(true) {
                if(!ssl_) {
                    ssl_ = std::make_shared<ssl_t>(io_service_);
                    auto connect_res = server_connector_.connect(
                        std::shared_ptr<ssl_stream_t>(ssl_, &ssl_->stream), host_, port_, yield, *resume_);
                    if(auto* ep = boost::get<std::exception_ptr>(&connect_res))
                        api_call_res.first = *ep;
                    else
                        api_call_res = do_api_call(req.call, req.api_token, req.request, yield);
                    break;
                } else {
                    api_call_res = do_api_call(req.call, req.api_token, req.request, yield);
                    if(!api_call_res.first) break;

                    // Server probably closed previous connection due to timeout
                    // Reconnect and retry API call
                    ssl_ = nullptr;
                }
            }

            req.callback(api_call_res.first, api_call_res.second);
            requests_.pop_front();
        }
    });
    (*resume_)();
}

void api_server_t::stop(std::function<void()> cb)
{
    if(resume_) stop();
    io_service_.post(std::move(cb));
}

void api_server_t::stop()
{
    if(ssl_->stream.next_layer().is_open())
        ssl_->stream.next_layer().close(); // TODO: shutdown gracefully
    server_connector_.cancel();
    resume_ = nullptr;
}

std::pair<std::exception_ptr, nlohmann::json>
api_server_t::do_api_call(const std::string& call, const std::string& api_token,
                          const nlohmann::json& request_json, coro_t::pull_type& yield)
{
    using namespace boost::beast;
    http::request<http::string_body> request;
    request.version(11);
    request.method(http::verb::post);
    request.target("/bot" + api_token + '/' + call);
    request.set(http::field::host, host_);
    request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    request.set(http::field::content_type, "application/json");
    auto body = request_json.dump();
    request.set(http::field::content_length, body.size());
    request.body() = std::move(body);

    BOOST_LOG_TRIVIAL(trace) << "Writing HTTP request: \n" << request;
    auto result = util::async_write(ssl_->stream, request, yield, *resume_, ssl_);
    if(auto ec = std::get<0>(result))
        return {std::make_exception_ptr(system_error{ec, "Cannot write HTTP request to API server: " + ec.message()}), {}};
    BOOST_LOG_TRIVIAL(debug) << "Wrote HTTP request; reading response";

    http::response<http::string_body> response;
    flat_buffer buffer;
    result = util::async_read(ssl_->stream, buffer, response, yield, *resume_, ssl_);
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
