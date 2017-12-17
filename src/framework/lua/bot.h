/*
 *  framework/lua_bot.h
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 03/12/2017.
 *
 */

#ifndef FRAMEWORK_LUA_BOT_H
#define FRAMEWORK_LUA_BOT_H

#include <boost/asio/io_service.hpp>

#include <lua.hpp>

#include "framework/api_server.h"
#include "framework/bot.h"
#include "framework/config.h"
#include "framework/longpoll_update_source.h"

namespace framework {
namespace lua {

struct globals_t;

class bot_t : public framework::bot_t, public util::callback_wrapper_t
{
public:
    bot_t(boost::asio::io_service& io_service,
          const bot_config_t& bot_config,
          const api_server_config_t& api_server_config,
          const lua_bot_config_t& lua_bot_config,
          api_server_t& api_server);
    ~bot_t();

public:
    api_server_t& api_server() { return api_server_; }
    longpoll_update_source_t& update_source() { return update_source_; }
    const bot_config_t& bot_config() { return bot_config_; }

private: // framework::bot_t
    void reload() final;
    void stop() final;

private:
    boost::asio::io_service& io_service_;
    const bot_config_t& bot_config_;
    const lua_bot_config_t& lua_bot_config_;
    api_server_t& api_server_;
    longpoll_update_source_t update_source_;

    boost::filesystem::path script_path_;

    std::unique_ptr<lua_State, void(*)(lua_State*)> lua_state_;
    std::unique_ptr<globals_t> globals_;
    lua_State* main_thread_;
    int main_thread_ref_;
};

} // namespace lua
} // namespace framework

#endif // FRAMEWORK_LUA_BOT_H
