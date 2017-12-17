/*
 *  framework/api_server.h
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 27/11/2017.
 *
 */

#ifndef FRAMEWORK_API_SERVER_H
#define FRAMEWORK_API_SERVER_H

#include <boost/asio/steady_timer.hpp>
#include <boost/coroutine2/all.hpp>
#include <boost/variant.hpp>

#include <nlohmann/json.hpp>

#include "framework/config.h"
#include "framework/server_connector.h"
#include "framework/service.h"
#include "util/asio.h"
#include "util/coroutine.h"

namespace framework {

class api_server_t : public service_t
{
public:
    using coro_t = boost::coroutines2::coroutine<void>;
    using cb_t = std::function<void(std::exception_ptr, nlohmann::json)>;
    using ssl_stream_t = util::ssl_stream_t;

    struct request_t
    {
        std::string api_token;
        std::string call;
        nlohmann::json request;
        cb_t callback;
    };

public:
    api_server_t(boost::asio::io_service& io_service,
                 const api_server_config_t& api_server_config);

    void call_api(request_t request);

private: // service_t
    void reload() final;
    void stop(std::function<void()> cb) final;

private:
    void stop();
    void ping(coro_t::pull_type& yield);

std::pair<std::exception_ptr, nlohmann::json>
    do_api_call(const std::string& call, const std::string& api_token,
                const nlohmann::json& request_json, coro_t::pull_type& yield);

private:
    struct ssl_t
    {
        ssl_t(boost::asio::io_service& io_service)
            : context{boost::asio::ssl::context::sslv23_client}
            , stream{io_service, context}
        {}

        boost::asio::ssl::context context;
        ssl_stream_t stream;
    };

    const api_server_config_t& api_server_config_;

    std::string host_;
    uint16_t port_;

    std::shared_ptr<ssl_t> ssl_;
    server_connector_t server_connector_;
    std::list<request_t> requests_;
    util::steady_timer_t ping_timer_;
    std::unique_ptr<util::push_coro_t> resume_;
};

} // namespace framework

#endif // FRAMEWORK_API_SERVER_H
