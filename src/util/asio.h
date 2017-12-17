/*
 *  util/asio.h
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 23/11/2017.
 *
 */

#ifndef UTIL_ASIO_H
#define UTIL_ASIO_H

#include <boost/any.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/http.hpp>
#include <boost/coroutine2/all.hpp>
#include <boost/variant.hpp>

#include "util/callback_wrapper.h"

namespace util {

template <typename T>
struct default_traits_t
{
    static void cancel(T& obj) { obj.cancel(); }
    static void close(T& obj) { obj.close(); }
};

template <typename T, typename Traits = default_traits_t<T>>
class cancellable_object_t : public T, public callback_wrapper_t
{
public:
    template <typename... Args>
    cancellable_object_t(Args&&... args) : T(std::forward<Args>(args)...) {}

    void cancel()
    {
        Traits::cancel(static_cast<T&>(*this));
        cancel_callbacks();
    }
};

template <typename T, typename Traits = default_traits_t<T>>
class closable_object_t : public cancellable_object_t<T, Traits>
{
public:
    template <typename... Args>
    closable_object_t(Args&&... args) : cancellable_object_t<T, Traits>(std::forward<Args>(args)...) {}

    void close()
    {
        Traits::close(static_cast<T&>(*this));
        this->cancel_callbacks();
    }
};

using steady_timer_t = cancellable_object_t<boost::asio::steady_timer>;
using signal_set_t = cancellable_object_t<boost::asio::signal_set>;
using tcp_socket_t = closable_object_t<boost::asio::ip::tcp::socket>;
using ssl_stream_t = boost::asio::ssl::stream<tcp_socket_t>;
using tcp_resolver_t = cancellable_object_t<boost::asio::ip::tcp::resolver>;

template <typename Yield, typename Resume>
inline boost::variant<boost::system::error_code, int>
async_wait_signal(signal_set_t& sigset,
                  Yield& yield,
                  Resume& resume,
                  boost::any bind_data = boost::any{})
{
    boost::system::error_code ec;
    int signo;

    sigset.async_wait(sigset.wrap(
        [&, bind_data](boost::system::error_code _ec, int _signo)
    {
        ec = _ec;
        signo = _signo;
        resume();
    }));
    yield();

    if(ec) return ec;
    return signo;
}

template <typename Yield, typename Resume>
boost::system::error_code
async_wait_timer(steady_timer_t& timer,
                 Yield& yield,
                 Resume& resume,
                 boost::any bind_data = boost::any{})
{
    boost::system::error_code ec;

    timer.async_wait(timer.wrap(
        [&, bind_data](boost::system::error_code _ec)
    {
        ec = _ec;
        resume();
    }));
    yield();

    return ec;
}

template <typename Yield, typename Resume>
boost::system::error_code
async_connect(tcp_socket_t& socket,
              const boost::asio::ip::tcp::endpoint& endpoint,
              Yield& yield,
              Resume& resume,
              boost::any bind_data = boost::any{})
{
    boost::system::error_code ec;

    socket.async_connect(endpoint, socket.wrap(
        [&, bind_data](boost::system::error_code _ec)
    {
        ec = _ec;
        resume();
    }));
    yield();

    return ec;
}

template <typename Yield, typename Resume>
boost::system::error_code
async_handshake(ssl_stream_t& ssl_stream,
                boost::asio::ssl::stream_base::handshake_type handshake_type,
                Yield& yield,
                Resume& resume,
                boost::any bind_data = boost::any{})
{
    boost::system::error_code ec;

    ssl_stream.async_handshake(handshake_type, ssl_stream.next_layer().wrap(
        [&, bind_data](boost::system::error_code _ec)
    {
        ec = _ec;
        resume();
    }));
    yield();

    return ec;
}

template <typename Yield, typename Resume>
boost::variant<boost::system::error_code, boost::asio::ip::tcp::resolver::results_type>
async_resolve(tcp_resolver_t& resolver,
              const std::string& host_name,
              uint16_t port,
              Yield& yield,
              Resume& resume,
              boost::any bind_data = boost::any{})
{
    using result_t = boost::asio::ip::tcp::resolver::results_type;
    boost::system::error_code ec;
    result_t resolver_result;

    resolver.async_resolve(host_name, std::to_string(port), resolver.wrap(
        [&, bind_data](boost::system::error_code _ec, result_t result_)
    {
        ec = _ec;
        resolver_result = std::move(result_);
        resume();
    }));
    yield();

    if(ec) return ec;
    return resolver_result;
}

template <typename Request, typename Yield, typename Resume>
std::tuple<boost::system::error_code, size_t>
async_write(ssl_stream_t& stream,
            Request& request,
            Yield& yield,
            Resume& resume,
            boost::any bind_data = boost::any{})
{
    boost::system::error_code ec;
    size_t transferred;

    boost::beast::http::async_write(stream, request, stream.next_layer().wrap(
        [&, bind_data](boost::system::error_code _ec, size_t _transferred)
    {
        ec = _ec;
        transferred = _transferred;
        resume();
    }));
    yield();

    return std::make_tuple(ec, transferred);
}

template <typename Buffer, typename Response, typename Yield, typename Resume>
std::tuple<boost::system::error_code, size_t>
async_read(ssl_stream_t& stream,
           Buffer& buffer,
           Response& response,
           Yield& yield,
           Resume& resume,
           boost::any bind_data = boost::any{})
{
    boost::system::error_code ec;
    size_t transferred;

    boost::beast::http::async_read(stream, buffer, response, stream.next_layer().wrap(
        [&, bind_data](boost::system::error_code _ec, size_t _transferred)
    {
        ec = _ec;
        transferred = _transferred;
        resume();
    }));
    yield();

    return std::make_tuple(ec, transferred);
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
