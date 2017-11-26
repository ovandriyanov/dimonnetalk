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
                                                   const longpoll_config_t& config,
                                                   std::string api_token,
                                                   std::function<void(nlohmann::json)> update_callback)
    : io_service_{io_service}
    , config_{config}
    , api_token_{std::move(api_token)}
    , update_callback_{std::move(update_callback)}
    , port_{0}
    , last_update_id_{0}
{
}

void longpoll_update_source_t::reload()
{
    assert(config_.host.size());
    if(config_.host == host_ || config_.port == port_) return;
    host_ = config_.host;
    port_ = config_.port;

    session_ = std::make_unique<session_t>(io_service_, [&](coro_t::push_type& yield) {
        while(true) {
            try {
                BOOST_LOG_TRIVIAL(debug) << "Resolving " << host_;
                boost::asio::ip::tcp::resolver resolver{io_service_};
                auto resolve_result = util::async_resolve(resolver, host_, port_, *this, yield, session_->resume);
                if(auto* ec = boost::get<error_code>(&resolve_result))
                    throw system_error{*ec, "Cannot resolve host name " + host_};

                auto& socket = ssl_->stream.next_layer();
                error_code ec;
                boost::asio::ip::tcp::endpoint endpoint;
                for(const auto& result : boost::get<boost::asio::ip::tcp::resolver::results_type>(resolve_result)) {
                    if(socket.open(result.endpoint().protocol(), ec)) {
                        BOOST_LOG_TRIVIAL(debug) << "Cannot open socket for endpoint " << result.endpoint() << ": " << ec.message();
                        continue;
                    }
                    BOOST_LOG_TRIVIAL(debug) << "Connecting to " << result.endpoint();
                    ec = util::async_connect(socket, result.endpoint(), *this, yield, resume_);
                    if(ec) {
                        BOOST_LOG_TRIVIAL(debug) << "Cannot connect to " << result.endpoint() << ": " << ec.message();
                        continue;
                    } else {
                        endpoint = result.endpoint();
                        BOOST_LOG_TRIVIAL(debug) << "Connected to " << endpoint << "; performing SSL handshake";
                        break;
                    }
                }
                if(!socket.is_open()) {
                    throw std::runtime_error{"Cannot connect to " + host_ + ':' + std::to_string(port_) +
                                             ": no more endpoints to try"};
                }

                using handshake_type = boost::asio::ssl::stream_base::handshake_type;
                ec = util::async_handshake(ssl_->stream, handshake_type::client, *this, yield, resume_);
                if(ec) {
                    throw system_error{ec, "Cannot connect to " + host_ + ':' + std::to_string(port_) +
                                           ": SSL handshake: " + ec.message()};
                }

                BOOST_LOG_TRIVIAL(debug) << "SSL handshake with " << endpoint << " complete";
                while(true) {
                    using namespace boost::beast;
                    http::request<http::empty_body> request;
                    request.version(11);
                    request.method(http::verb::get);
                    request.target("/bot" + api_token_ + "/getupdates?timeout=" + std::to_string(config_.poll_timeout));
                    request.set(http::field::host, host_);
                    request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

                    auto result = util::async_write(ssl_->stream, request, *this, yield, resume_);
                    if(auto ec = std::get<0>(result)) {
                        throw system_error{ec, "Cannot write HTTP request to " +
                                                endpoint.address().to_string() + ':' + std::to_string(endpoint.port()) +
                                                ": " + ec.message()};
                    }
                    BOOST_LOG_TRIVIAL(trace) << "Wrote HTTP request: \n" << request;

                    http::response<http::string_body> response;
                    flat_buffer buffer;
                    result = util::async_read(ssl_->stream, buffer, response, *this, yield, resume_);
                    if(auto ec = std::get<0>(result)) {
                        throw system_error{ec, "Cannot read HTTP response from " +
                                                endpoint.address().to_string() + ':' + std::to_string(endpoint.port()) +
                                                ": " + ec.message()};
                    }
                    BOOST_LOG_TRIVIAL(trace) << "Got HTTP response: \n" << response;

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
                        for(auto& item : response_json.at("result")) {
                            int update_id = item.at("update_id");
                            if(last_update_id_ > update_id)
                                continue;
                            last_update_id_ = update_id;
                            update_callback_(std::move(item));
                        }
                    } catch(const nlohmann::json::exception& ex) {
                        throw http_error(ex.what());
                    }
                }
            } catch(const std::runtime_error& ex) {
                if(stop_callback_) {
                    // TODO: shutdown gracefully
                    ssl_ = nullptr;
                    return io_service_.post(std::move(stop_callback_));
                }

                BOOST_LOG_TRIVIAL(info) << ex.what();
                ssl_ = std::make_unique<ssl_t>(io_service_);
                boost::asio::steady_timer timer{io_service_};
                timer.expires_from_now(std::chrono::seconds{config_.retry_timeout});
                util::async_wait(timer, *this, yield, resume_);
            }
        }
    });
}

void longpoll_update_source_t::stop(std::function<void()> cb)
{
    stop_callback_ = std::move(cb);

    ssl_->stream.next_layer().cancel();
    timer_.cancel();
}

} // namespace framework
