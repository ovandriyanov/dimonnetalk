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
#include <boost/asio/steady_timer.hpp>
#include <boost/beast.hpp>
#include <boost/coroutine2/all.hpp>

#include <nlohmann/json.hpp>

#include "framework/config.h"

namespace framework {

class longpoll_update_source_t
{
public:
    longpoll_update_source_t(boost::asio::io_service& io_service,
                             const longpoll_config_t& config,
                             std::string api_token,
                             std::function<void(nlohmann::json)> update_callback);

    const std::string& api_token() const { return api_token_; }

    void reload();
    void stop(std::function<void()> cb);

private:
    using coro_t = boost::coroutines2::coroutine<void>;
    class coroutine_t : public std::enable_shared_from_this<coroutine_t>
    {
    public:
        coroutine_t(longpoll_update_source_t& longpoll);

        template <typename T>
        std::shared_ptr<T> make_shared(T& field) { return std::shared_ptr<T>(shared_from_this(), &field); }

    public:
        longpoll_update_source_t& longpoll_;
        boost::asio::ssl::context ssl_context;
        boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_stream;
        boost::asio::steady_timer timer;
        bool stop;
        coro_t::pull_type resume;
    };

private:
    void stop(coroutine_t& coro);

private:
    boost::asio::io_service& io_service_;
    const longpoll_config_t& config_;
    const std::string api_token_;
    std::function<void(nlohmann::json)> update_callback_;

    std::string host_;
    uint16_t port_;

    std::shared_ptr<coroutine_t> coroutine_;
    int last_update_id_;
};

} // namespace framework

#endif // FRAMEWORK_LONGPOLL_UPDATE_SOURCE_H
