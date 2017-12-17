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

#include "framework/bot.h"
#include "framework/config.h"

namespace framework {
namespace lua {

class bot_t : public framework::bot_t
{
public:
    bot_t(boost::asio::io_service& io_service,
              const bot_config_t& bot_config,
              const lua_bot_config_t& lua_bot_config);

private: // framework::bot_t
    void reload() final;
    void stop() final;

private:
    boost::asio::io_service& io_service_;
    const bot_config_t& bot_config_;
    const lua_bot_config_t& lua_bot_config_;

    boost::filesystem::path script_path_;

    std::unique_ptr<lua_State, void(*)(lua_State*)> lua_state_;
    lua_State* main_thread_;
    int main_thread_ref_;
};

} // namespace lua
} // namespace framework

#endif // FRAMEWORK_LUA_BOT_H
