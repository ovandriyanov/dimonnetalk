/*
 *  util/asio.h
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 23/11/2017.
 *
 */

#ifndef UTIL_ASIO_H
#define UTIL_ASIO_H

#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/coroutine2/all.hpp>

#include "util/callback_wrapper.h"

namespace util {

template <typename T>
using coro_t = boost::coroutines2::coroutine<T>;

class signal_set_t : private callback_wrapper_t
{
public:
    signal_set_t(boost::asio::io_service& io_service);
    signal_set_t(boost::asio::io_service& io_service, int signo);
    signal_set_t(boost::asio::io_service& io_service, int signo1, int signo2);

    void add(int signal);
    void clear();

    int async_wait(coro_t<void>::push_type& yield, coro_t<void>::pull_type& resume);
    void cancel();
    void cancel(coro_t<void>::pull_type& resume);

private:
    boost::asio::signal_set sigset_;
};

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
