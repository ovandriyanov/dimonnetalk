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
#include <boost/asio/signal_set.hpp>
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
