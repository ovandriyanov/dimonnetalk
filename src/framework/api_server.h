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
#include "framework/service.h"

namespace framework {

class api_server_t : public service_t
{
public:
    using coro_t = boost::coroutines2::coroutine<void>;
public:
    api_server_t(boost::asio::io_service& io_service,
                 const api_server_config_t& api_server_config);

    boost::variant<std::exception_ptr, nlohmann::json>
        call_api(const std::string& api_token, const std::string& call, coro_t::push_type& yield, coro_t::pull_type& resume);

private: // service_t
    void reload() final;
    void stop(std::function<void()> cb) final;

private:
    class coroutine_t : public std::enable_shared_from_this<coroutine_t>
    {
    public:
        coroutine_t(api_server_t& api_server);

    public:
        api_server_t& api_server;

        coro_t::pull_type resume;
    };
};

} // namespace framework

#endif // FRAMEWORK_API_SERVER_H
