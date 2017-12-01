/*
 *  framework/server_connector.h
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 27/11/2017.
 *
 */

#ifndef FRAMEWORK_SERVER_CONNECTOR_H
#define FRAMEWORK_SERVER_CONNECTOR_H

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/coroutine2/all.hpp>
#include <boost/variant.hpp>

#include "util/coroutine.h"

namespace framework {

class server_connector_t
{
public:
    using coro_t = boost::coroutines2::coroutine<void>;
    using ssl_stream_t = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;

public:
    server_connector_t(boost::asio::io_service& io_service);

    boost::variant<std::exception_ptr, boost::asio::ip::tcp::endpoint>
        connect(const std::shared_ptr<ssl_stream_t>& ssl_stream,
                const std::string& host, uint16_t port,
                coro_t::pull_type& yield, util::push_coro_t& resume);
    void cancel();

private:
    boost::asio::ip::tcp::resolver resolver_;
};

} // namespace framework

#endif // FRAMEWORK_SERVER_CONNECTOR_H
