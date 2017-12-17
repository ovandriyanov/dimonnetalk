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
#include "framework/server_connector.h"
#include "util/coroutine.h"

namespace framework {

class longpoll_update_source_t
{
public:
    longpoll_update_source_t(boost::asio::io_service& io_service,
                             const longpoll_config_t& longpoll_config,
                             const api_server_config_t& api_server_config,
                             std::string api_token,
                             std::function<void(nlohmann::json)> update_callback);

    const std::string& api_token() const { return api_token_; }

    void reload();
    void stop(std::function<void()> cb);

private:
    void stop();

private:
    using ssl_stream_t = util::ssl_stream_t;
    struct ssl_t
    {
        ssl_t(boost::asio::io_service& io_service)
            : context{boost::asio::ssl::context::sslv23_client}
            , stream{io_service, context}
        {}

        boost::asio::ssl::context context;
        ssl_stream_t stream;
    };

    boost::asio::io_service& io_service_;
    const longpoll_config_t& longpoll_config_;
    const api_server_config_t& api_server_config_;

    const std::string api_token_;
    std::function<void(nlohmann::json)> update_callback_;

    std::string host_;
    uint16_t port_;

    std::shared_ptr<ssl_t> ssl_;
    server_connector_t server_connector_;
    util::steady_timer_t timer_;
    std::unique_ptr<util::push_coro_t> resume_;
    int last_update_id_;
};

} // namespace framework

#endif // FRAMEWORK_LONGPOLL_UPDATE_SOURCE_H
