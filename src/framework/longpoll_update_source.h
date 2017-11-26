/*
 *  framework/longpoll_update_source.h
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 25/11/2017.
 *
 */

#ifndef FRAMEWORK_LONGPOLL_UPDATE_SOURCE_H
#define FRAMEWORK_LONGPOLL_UPDATE_SOURCE_H

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/coroutine2/all.hpp>

#include "framework/config.h"
#include "util/callback_wrapper.h"

namespace framework {

class longpoll_update_source_t : private util::callback_wrapper_t
{
public:
    longpoll_update_source_t(boost::asio::io_service& io_service,
                             const longpoll_config_t& config,
                             std::string api_token);

    const std::string& api_token() const { return api_token_; }

    void reload();
    void stop(std::function<void()> cb);

private:
    using coro_t = boost::coroutines2::coroutine<void>;
    struct ssl_t {
        ssl_t(boost::asio::io_service& io_service)
            : context{boost::asio::ssl::context::sslv23_client}, stream{io_service, context} {}

        boost::asio::ssl::context context;
        boost::asio::ssl::stream<boost::asio::ip::tcp::socket> stream;
    };
    struct finishing_t {
        ssl_t ssl;
        std::unique_ptr<coro_t::pull_type> resume;
    };

private:
    boost::asio::io_service& io_service_;
    const longpoll_config_t& config_;
    const std::string api_token_;

    std::string host_;
    uint16_t port_;

    std::unique_ptr<ssl_t> ssl_;
    std::unique_ptr<coro_t::pull_type> resume_;
    std::list<std::unique_ptr<finishing_t>> finishing_;
};

} // namespace framework

#endif // FRAMEWORK_LONGPOLL_UPDATE_SOURCE_H
