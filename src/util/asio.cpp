/*
 *  util/asio.cpp
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 23/11/2017.
 *
 */

#include "util/asio.h"

namespace util {

using namespace boost::system;

signal_set_t::signal_set_t(boost::asio::io_service& io_service)
    : sigset_{io_service}
{
}

signal_set_t::signal_set_t(boost::asio::io_service& io_service, int signo)
    : sigset_{io_service}
{
    add(signo);
}

signal_set_t::signal_set_t(boost::asio::io_service& io_service, int signo1, int signo2)
    : sigset_{io_service}
{
    add(signo1);
    add(signo2);
}

void signal_set_t::add(int signal)
{
    error_code ec;
    sigset_.add(signal, ec);
    if(ec) throw system_error{ec};
}

void signal_set_t::clear()
{
    error_code ec;
    sigset_.clear(ec);
    if(ec) throw system_error{ec};
}

int signal_set_t::async_wait(coro_t<void>::push_type& yield, boost::optional<coro_t<void>::pull_type>& resume)
{
    error_code ec;
    int signo;

    sigset_.async_wait(wrap([&](error_code _ec, int _signo) {
        ec = _ec;
        signo = _signo;
        (*resume)();
    }));
    yield();

    if(ec) throw system_error{ec};
    return signo;
}

void signal_set_t::cancel()
{
    sigset_.cancel();
    cancel_callbacks();
}

void signal_set_t::cancel(coro_t<void>::pull_type& resume)
{
    cancel();
    resume();
}


error_code async_connect(boost::asio::ip::tcp::socket& socket,
                         const boost::asio::ip::tcp::endpoint& endpoint,
                         callback_wrapper_t& callback_wrapper,
                         coro_t<void>::push_type& yield,
                         coro_t<void>::pull_type& resume)
{
    error_code ec;
    socket.async_connect(endpoint, callback_wrapper.wrap([&](error_code _ec) {
        ec = _ec;
        resume();
    }));
    yield();
    return ec;
}

} // namespace util
