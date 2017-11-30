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

namespace util {

template <typename Yield, typename Resume>
inline boost::variant<boost::system::error_code, int>
async_wait_signal(boost::asio::signal_set& sigset,
                  Yield& yield,
                  Resume& resume,
                  boost::any bind_data = boost::any{})
{
    boost::system::error_code ec;
    int signo;

    sigset.async_wait(resume.wrap(
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
async_wait_timer(boost::asio::steady_timer& timer,
                 Yield& yield,
                 Resume& resume,
                 boost::any bind_data = boost::any{})
{
    boost::system::error_code ec;

    timer.async_wait(resume.wrap(
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
async_connect(boost::asio::ip::tcp::socket& socket,
              const boost::asio::ip::tcp::endpoint& endpoint,
              Yield& yield,
              Resume& resume,
              boost::any bind_data = boost::any{})
{
    boost::system::error_code ec;

    socket.async_connect(endpoint, resume.wrap(
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
async_handshake(boost::asio::ssl::stream<boost::asio::ip::tcp::socket>& ssl_stream,
                boost::asio::ssl::stream_base::handshake_type handshake_type,
                Yield& yield,
                Resume& resume,
                boost::any bind_data = boost::any{})
{
    boost::system::error_code ec;

    ssl_stream.async_handshake(handshake_type, resume.wrap(
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
async_resolve(boost::asio::ip::tcp::resolver& resolver,
              const std::string& host_name,
              uint16_t port,
              Yield& yield,
              Resume& resume,
              boost::any bind_data = boost::any{})
{
    using result_t = boost::asio::ip::tcp::resolver::results_type;
    boost::system::error_code ec;
    result_t resolver_result;

    resolver.async_resolve(host_name, std::to_string(port), resume.wrap(
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

template <typename Stream, typename Request, typename Yield, typename Resume>
std::tuple<boost::system::error_code, size_t>
async_write(Stream& stream,
            Request& request,
            Yield& yield,
            Resume& resume,
            boost::any bind_data = boost::any{})
{
    boost::system::error_code ec;
    size_t transferred;

    boost::beast::http::async_write(stream, request, resume.wrap(
        [&, bind_data](boost::system::error_code _ec, size_t _transferred)
    {
        ec = _ec;
        transferred = _transferred;
        resume();
    }));
    yield();

    return std::make_tuple(ec, transferred);
}

template <typename Stream, typename Buffer, typename Response, typename Yield, typename Resume>
std::tuple<boost::system::error_code, size_t>
async_read(Stream& stream,
           Buffer& buffer,
           Response& response,
           Yield& yield,
           Resume& resume,
           boost::any bind_data = boost::any{})
{
    boost::system::error_code ec;
    size_t transferred;

    boost::beast::http::async_read(stream, buffer, response, resume.wrap(
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
