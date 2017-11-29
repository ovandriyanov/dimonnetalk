/*
 *  framework/server_connector.cpp
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 27/11/2017.
 *
 */

#include <boost/log/trivial.hpp>

#include "framework/server_connector.h"
#include "util/asio.h"
#include <boost/any.hpp>

namespace framework {

using namespace boost::system;

server_connector_t::server_connector_t(boost::asio::io_service& io_service)
    : resolver_{io_service}
{
}

boost::variant<std::exception_ptr, boost::asio::ip::tcp::endpoint>
server_connector_t::connect(const std::shared_ptr<ssl_stream_t>& ssl_stream,
                            const std::string& host, uint16_t port,
                            coro_t::pull_type& yield, coro_t::push_type& resume)
{
    BOOST_LOG_TRIVIAL(debug) << "Resolving " << host;
    auto resolve_result = util::async_resolve(resolver_, host, port, yield, resume);
    if(auto* ec = boost::get<error_code>(&resolve_result))
        return std::make_exception_ptr(system_error{*ec, "Cannot resolve host name " + host});

    auto& socket = ssl_stream->next_layer();
    error_code ec;
    boost::asio::ip::tcp::endpoint endpoint;
    for(const auto& result : boost::get<boost::asio::ip::tcp::resolver::results_type>(resolve_result)) {
        if(socket.open(result.endpoint().protocol(), ec)) {
            BOOST_LOG_TRIVIAL(debug) << "Cannot open socket for endpoint " << result.endpoint() << ": " << ec.message();
            continue;
        }
        BOOST_LOG_TRIVIAL(debug) << "Connecting to " << result.endpoint();
        ec = util::async_connect(socket, result.endpoint(), yield, resume);
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
        return std::make_exception_ptr(std::runtime_error{"Cannot connect to " + host + ':' + std::to_string(port) +
                                       ": no more endpoints to try"});
    }

    using handshake_type = boost::asio::ssl::stream_base::handshake_type;
    ec = util::async_handshake(*ssl_stream, handshake_type::client, yield, resume, ssl_stream);
    if(ec) {
        return std::make_exception_ptr(system_error{ec, "Cannot connect to " + host + ':' + std::to_string(port) +
                                       ": SSL handshake: " + ec.message()});
    }
    BOOST_LOG_TRIVIAL(debug) << "SSL handshake with " << endpoint << " complete";
    return endpoint;
}

void server_connector_t::cancel()
{
    resolver_.cancel();
}

} // namespace framework
