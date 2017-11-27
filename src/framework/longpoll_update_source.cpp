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

    if(coroutine_) stop(*coroutine_);
    coroutine_ = std::make_shared<coroutine_t>(*this);
    coroutine_->resume();
}

void longpoll_update_source_t::stop(std::function<void()> cb)
{
    if(coroutine_) stop(*coroutine_);
    io_service_.post(std::move(cb));
}

void longpoll_update_source_t::stop(coroutine_t& coro)
{
    coroutine_->ssl_stream.next_layer().cancel();
    coroutine_->timer.cancel();
    coroutine_->stop = true;
}

longpoll_update_source_t::coroutine_t::coroutine_t(longpoll_update_source_t& longpoll)
    : longpoll_{longpoll}
    , ssl_context{boost::asio::ssl::context::sslv23_client}
    , ssl_stream{longpoll.io_service_, ssl_context}
    , timer(longpoll.io_service_)
    , stop{false}
    , resume{[&](coro_t::push_type& yield)
{
    yield();
    while(true) {
        try {
            BOOST_LOG_TRIVIAL(debug) << "Resolving " << longpoll_.host_;
            boost::asio::ip::tcp::resolver resolver{longpoll_.io_service_};
            auto resolve_result = util::async_resolve(make_shared(resolver), longpoll_.host_, longpoll_.port_, yield, resume);
            if(stop) return;
            if(auto* ec = boost::get<error_code>(&resolve_result))
                throw system_error{*ec, "Cannot resolve host name " + longpoll_.host_};

            auto& socket = ssl_stream.next_layer();
            error_code ec;
            boost::asio::ip::tcp::endpoint endpoint;
            for(const auto& result : boost::get<boost::asio::ip::tcp::resolver::results_type>(resolve_result)) {
                if(socket.open(result.endpoint().protocol(), ec)) {
                    BOOST_LOG_TRIVIAL(debug) << "Cannot open socket for endpoint " << result.endpoint() << ": " << ec.message();
                    continue;
                }
                BOOST_LOG_TRIVIAL(debug) << "Connecting to " << result.endpoint();
                ec = util::async_connect(make_shared(socket), result.endpoint(), yield, resume);
                if(stop) return;
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
                throw std::runtime_error{"Cannot connect to " + longpoll_.host_ + ':' + std::to_string(longpoll_.port_) +
                                         ": no more endpoints to try"};
            }

            using handshake_type = boost::asio::ssl::stream_base::handshake_type;
            ec = util::async_handshake(make_shared(ssl_stream), handshake_type::client, yield, resume);
            if(stop) return;
            if(ec) {
                throw system_error{ec, "Cannot connect to " + longpoll_.host_ + ':' + std::to_string(longpoll_.port_) +
                                       ": SSL handshake: " + ec.message()};
            }

            BOOST_LOG_TRIVIAL(debug) << "SSL handshake with " << endpoint << " complete";
            while(true) {
                using namespace boost::beast;
                http::request<http::empty_body> request;
                request.version(11);
                request.method(http::verb::get);
                request.target("/bot" + longpoll_.api_token_ +
                               "/getupdates"
                               "?timeout=" + std::to_string(longpoll_.config_.poll_timeout) +
                               "&offset=" + std::to_string(longpoll_.last_update_id_ + 1));
                request.set(http::field::host, longpoll_.host_);
                request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

                auto result = util::async_write(make_shared(ssl_stream), request, yield, resume);
                if(stop) return;
                if(auto ec = std::get<0>(result)) {
                    throw system_error{ec, "Cannot write HTTP request to " +
                                            endpoint.address().to_string() + ':' + std::to_string(endpoint.port()) +
                                            ": " + ec.message()};
                }
                BOOST_LOG_TRIVIAL(trace) << "Wrote HTTP request: \n" << request;

                http::response<http::string_body> response;
                flat_buffer buffer;
                result = util::async_read(make_shared(ssl_stream), buffer, response, yield, resume);
                if(stop) return;
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
                        if(longpoll_.last_update_id_ > update_id)
                            continue;
                        longpoll_.last_update_id_ = update_id;
                        longpoll_.update_callback_(std::move(item));
                    }
                } catch(const nlohmann::json::exception& ex) {
                    throw http_error(ex.what());
                }
            }
        } catch(const std::runtime_error& ex) {
            BOOST_LOG_TRIVIAL(info) << ex.what();
            if(ssl_stream.next_layer().is_open())
                ssl_stream.next_layer().close(); // TODO: shutdown gracefully
            timer.expires_from_now(std::chrono::seconds{longpoll_.config_.retry_timeout});
            auto ec = util::async_wait_timer(make_shared(timer), yield, resume);
            if(stop) return;
            if(ec) throw system_error{ec, "timer"};
        }
    }
}}
{
}

} // namespace framework
