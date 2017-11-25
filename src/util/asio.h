/*
 *  util/asio.h
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 23/11/2017.
 *
 */

#ifndef UTIL_ASIO_H
#define UTIL_ASIO_H

#include <boost/asio/buffer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/coroutine2/all.hpp>
#include <boost/variant.hpp>

#include "util/callback_wrapper.h"

namespace util {

template <typename CoroutinePtr>
boost::variant<boost::system::error_code, int>
async_wait(boost::asio::signal_set& sigset,
           util::callback_wrapper_t& callback_wrapper,
           boost::coroutines2::coroutine<void>::push_type& yield,
           CoroutinePtr& resume)
{
    boost::system::error_code ec;
    int signo;

    sigset.async_wait(callback_wrapper.wrap([&](boost::system::error_code _ec, int _signo) {
        ec = _ec;
        signo = _signo;
        (*resume)();
    }));
    yield();

    if(ec) return ec;
    return signo;
}

template <typename CoroutinePtr>
boost::system::error_code
async_connect(boost::asio::ip::tcp::socket& socket,
              const boost::asio::ip::tcp::endpoint& endpoint,
              util::callback_wrapper_t& callback_wrapper,
              boost::coroutines2::coroutine<void>::push_type& yield,
              CoroutinePtr& resume)
{
    boost::system::error_code ec;

    socket.async_connect(callback_wrapper.wrap([&](boost::system::error_code _ec) {
        ec = _ec;
        (*resume)();
    }));
    yield();

    return ec;
}

template <typename CoroutinePtr>
boost::system::error_code
async_handshake(boost::asio::ssl::stream<boost::asio::ip::tcp::socket>& ssl_stream,
                util::callback_wrapper_t& callback_wrapper,
                boost::coroutines2::coroutine<void>::push_type& yield,
                CoroutinePtr& resume)
{
    boost::system::error_code ec;

    ssl_stream.async_handshake(callback_wrapper.wrap([&](boost::system::error_code _ec) {
        ec = _ec;
        (*resume)();
    }));
    yield();

    return ec;
}

} // namespace util

namespace boost {
namespace asio {

inline const char* begin(const boost::asio::const_buffer& buf)
{
    return boost::asio::buffer_cast<const char*>(buf);
}

inline const char* end(const boost::asio::const_buffer& buf)
{
    return boost::asio::buffer_cast<const char*>(buf) + boost::asio::buffer_size(buf);
}

} // namespace asio
} // namespace boost

#endif // UTIL_ASIO_H
