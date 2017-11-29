/*
 *  framework/api_server.h
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 27/11/2017.
 *
 */

#ifndef FRAMEWORK_API_SERVER_H
#define FRAMEWORK_API_SERVER_H

#include <boost/coroutine2/all.hpp>
#include <boost/variant.hpp>

#include <nlohmann/json.hpp>

#include "framework/config.h"
#include "framework/server_connector.h"
#include "framework/service.h"
#include "util/coroutine.h"

namespace framework {

class api_server_t : public service_t
{
public:
    using coro_t = boost::coroutines2::coroutine<void>;
    using cb_t = std::function<void(std::exception_ptr, nlohmann::json)>;
    using ssl_stream_t = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;

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

private:
    const api_server_config_t& api_server_config_;

    std::string host_;
    uint16_t port_;

    boost::asio::ssl::context ssl_context_;
    std::shared_ptr<ssl_stream_t> ssl_stream_;
    server_connector_t server_connector_;
    std::list<request_t> requests_;
    std::unique_ptr<util::push_coro_t> resume_;
};

} // namespace framework

#endif // FRAMEWORK_API_SERVER_H
