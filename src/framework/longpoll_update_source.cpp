/*
 *  framework/longpoll_update_source.cpp
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 25/11/2017.
 *
 */

#include <boost/beast.hpp>
#include <boost/log/trivial.hpp>

#include "framework/longpoll_update_source.h"
#include "util/asio.h"

namespace framework {

using namespace boost::system;

longpoll_update_source_t::longpoll_update_source_t(boost::asio::io_service& io_service,
                                                   const longpoll_config_t& longpoll_config,
                                                   const api_server_config_t& api_server_config,
                                                   std::string api_token)
    : io_service_{io_service}
    , longpoll_config_{longpoll_config}
    , api_server_config_{api_server_config}
    , api_token_{std::move(api_token)}
    , port_{0}
    , server_connector_{io_service}
    , timer_(io_service)
    , last_update_id_{0}
{
}

void longpoll_update_source_t::reload()
{
    if(resume_) {
        if(api_server_config_.host == host_ || api_server_config_.port == port_)
            return;
    }

    host_ = api_server_config_.host;
    port_ = api_server_config_.port;

    if(resume_)
        stop();

    resume_ = std::make_unique<util::push_coro_t>(
        [&](boost::coroutines2::coroutine<void>::pull_type& yield)
    {
        while(true) {
            try {
                auto ssl = std::make_shared<ssl_t>(io_service_);
                auto connect_result = server_connector_.connect(
                    std::shared_ptr<ssl_stream_t>{ssl, &ssl->stream}, host_, port_, yield, *resume_);
                if(auto* ep = boost::get<std::exception_ptr>(&connect_result))
                    std::rethrow_exception(*ep);

                ssl_ = std::move(ssl);
                const auto endpoint = boost::get<boost::asio::ip::tcp::endpoint>(connect_result);

                while(true) {
                    if(callbacks_.empty()) yield();
                    assert(callbacks_.size());

                    using namespace boost::beast;
                    http::request<http::empty_body> request;
                    request.version(11);
                    request.method(http::verb::get);
                    request.target("/bot" + api_token_ +
                                   "/getupdates"
                                   "?timeout=" + std::to_string(longpoll_config_.poll_timeout) +
                                   "&offset=" + std::to_string(last_update_id_ + 1));
                    request.set(http::field::host, host_);
                    request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

                    auto result = util::async_write(ssl_->stream, request, yield, *resume_, ssl_);
                    if(auto ec = std::get<0>(result)) {
                        throw system_error{ec, "Cannot write HTTP request to " +
                                                endpoint.address().to_string() + ':' + std::to_string(endpoint.port()) +
                                                ": " + ec.message()};
                    }
                    // BOOST_LOG_TRIVIAL(trace) << "Wrote HTTP request: \n" << request;

                    http::response<http::string_body> response;
                    flat_buffer buffer;
                    result = util::async_read(ssl_->stream, buffer, response, yield, *resume_, ssl_);
                    if(auto ec = std::get<0>(result)) {
                        throw system_error{ec, "Cannot read HTTP response from " +
                                                endpoint.address().to_string() + ':' + std::to_string(endpoint.port()) +
                                                ": " + ec.message()};
                    }
                    // BOOST_LOG_TRIVIAL(trace) << "Got HTTP response: \n" << response;

                    auto http_error = [](const std::string& what) {
                        return std::runtime_error{"Cannot process HTTP response: " + what};
                    };

                    if(response.result() != http::status::ok)
                        throw http_error("Server returned HTTP status " + std::to_string(static_cast<int>(response.result())));

                    auto content_type = response["Content-Type"];
                    if(content_type != "application/json")
                        throw http_error("Invalid content type: " + std::string{content_type});

                    try {
                        auto response_json = nlohmann::json::parse(response.body());
                        if(response_json.at("ok") != true)
                            throw http_error("Field 'ok' is not 'True'");

                        for(auto& item : response_json.at("result"))
                            pending_updates_.emplace_back(std::move(item));

                        process_pending_updates();
                    } catch(const nlohmann::json::exception& ex) {
                        throw http_error(ex.what());
                    }
                }
            } catch(const std::runtime_error& ex) {
                BOOST_LOG_TRIVIAL(info) << ex.what();
                io_service_.post(std::bind(std::move(std::move(callbacks_.front())), std::current_exception(), nlohmann::json{}));
                callbacks_.pop_front();
                ssl_ = nullptr; // TODO: shutdown gracefully
                timer_.expires_from_now(std::chrono::seconds{longpoll_config_.retry_timeout});
                auto ec = util::async_wait_timer(timer_, yield, *resume_);
                if(ec) throw system_error{ec, "timer"};
            }
        }
    });
    (*resume_)();
}

void longpoll_update_source_t::stop(std::function<void()> cb)
{
    if(resume_) stop();
    io_service_.post(std::move(cb));
}

void longpoll_update_source_t::get_update(cb_t callback)
{
    callbacks_.emplace_back(std::move(callback));
    process_pending_updates();
    if(callbacks_.size() == 1 && ssl_) (*resume_)();
}

void longpoll_update_source_t::stop()
{
    if(ssl_) {
        // TODO: shutdown gracefully
        if(ssl_->stream.next_layer().is_open())
            ssl_->stream.next_layer().close();
        ssl_ = nullptr;
    }
    server_connector_.cancel();
    timer_.cancel();
    last_update_id_ = 0;
    resume_ = nullptr;
}

void longpoll_update_source_t::process_pending_updates()
{
    while(callbacks_.size() && pending_updates_.size()) {
        auto item = std::move(pending_updates_.front());
        pending_updates_.pop_front();
        int update_id = item.at("update_id");
        if(last_update_id_ > update_id)
            continue;
        last_update_id_ = update_id;

        assert(callbacks_.size());
        io_service_.post(std::bind(std::move(callbacks_.front()), nullptr, std::move(item)));
        callbacks_.pop_front();
    }
}

} // namespace framework
