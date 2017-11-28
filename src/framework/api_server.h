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

namespace framework {

class api_server_t : public service_t
{
public:
    using coro_t = boost::coroutines2::coroutine<void>;
    using cb_t = std::function<void(std::exception_ptr, nlohmann::json)>;

public:
    api_server_t(boost::asio::io_service& io_service,
                 const api_server_config_t& api_server_config);

    void call_api(std::string api_token, std::string call, nlohmann::json request, cb_t callback);

private:
    struct request_t
    {
        std::string api_token;
        std::string call;
        nlohmann::json request;
        cb_t callback;
    };

    class coroutine_t : public std::enable_shared_from_this<coroutine_t>
    {
    public:
        coroutine_t(api_server_t& api_server, std::list<request_t> requests);

        template <typename T>
        std::shared_ptr<T> make_shared(T& field) { return std::shared_ptr<T>(shared_from_this(), &field); }

        std::pair<std::exception_ptr, nlohmann::json>
            do_api_call(const std::string& call, const std::string& api_token, const nlohmann::json& request_json,
                        coro_t::push_type& yield, coro_t::pull_type& resume);

    public:
        api_server_t& api_server;
        boost::asio::ssl::context ssl_context;
        boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_stream;
        server_connector_t server_connector;
        std::list<request_t> requests;
        boost::variant<std::exception_ptr, nlohmann::json> result;
        bool stop;
        coro_t::pull_type resume;
    };

private: // service_t
    void reload() final;
    void stop(std::function<void()> cb) final;

private:
    void stop(coroutine_t& coro);

private:
    const api_server_config_t& api_server_config_;

    std::string host_;
    uint16_t port_;

    std::shared_ptr<coroutine_t> coroutine_;
};

} // namespace framework

#endif // FRAMEWORK_API_SERVER_H
