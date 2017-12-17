/*
 *  framework/lua_bot.cpp
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 03/12/2017.
 *
 */

#include <boost/asio/steady_timer.hpp>
#include <boost/log/trivial.hpp>

#include "framework/lua/bot.h"
#include "framework/lua/io.h"
#include "framework/lua/log.h"
#include "util/asio.h"

namespace framework {
namespace lua {

using namespace boost::system;

bot_t::bot_t(boost::asio::io_service& io_service,
             const bot_config_t& bot_config,
             const lua_bot_config_t& lua_bot_config)
    : io_service_{io_service}
    , bot_config_{bot_config}
    , lua_bot_config_{lua_bot_config}
    , lua_state_{nullptr, lua_close}
    , main_thread_{nullptr}
    , main_thread_ref_{0}
{
}

// framework::bot_t
void bot_t::reload()
{
    if(lua_state_) {
        if(script_path_ == lua_bot_config_.script_path)
            return;
    }

    script_path_ = lua_bot_config_.script_path;

    lua_state_.reset(luaL_newstate());
    if(!lua_state_) throw std::runtime_error{"Cannot create Lua state"};

    luaL_openlibs(lua_state_.get());
    if(int ec = luaL_loadfile(lua_state_.get(), script_path_.c_str())) {
        throw std::runtime_error{"Cannot load Lua script file " + script_path_.string() +
                                 ": " + lua_tostring(lua_state_.get(), -1)};
    }

    lua_pushlightuserdata(lua_state_.get(), &io_service_);
    lua_setfield(lua_state_.get(), LUA_REGISTRYINDEX, "io_service");

    luaL_requiref(lua_state_.get(), "dimonnetalk.log", push_log_table, false);
    lua_pop(lua_state_.get(), 1);

    luaL_requiref(lua_state_.get(), "dimonnetalk.io", push_io_table, false);
    lua_pop(lua_state_.get(), 1);

    int main_function_ref = luaL_ref(lua_state_.get(), LUA_REGISTRYINDEX);
    main_thread_ = lua_newthread(lua_state_.get());
    main_thread_ref_ = luaL_ref(lua_state_.get(), LUA_REGISTRYINDEX);
    lua_rawgeti(main_thread_, LUA_REGISTRYINDEX, main_function_ref);
    luaL_unref(lua_state_.get(), LUA_REGISTRYINDEX, main_function_ref);

    int ret = lua_resume(main_thread_, nullptr, 0);
    if(ret != LUA_OK && ret != LUA_YIELD) {
        throw std::runtime_error{"Cannot execute Lua script file " + script_path_.string() +
                                 ": " + lua_tostring(main_thread_, -1)};
    }
}

void bot_t::stop()
{
    lua_state_ = nullptr;
}

} // namespace lua
} // namespace framework
