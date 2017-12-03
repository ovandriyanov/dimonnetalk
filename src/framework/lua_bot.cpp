/*
 *  framework/lua_bot.cpp
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 03/12/2017.
 *
 */

#include "framework/lua_bot.h"

#include <boost/log/trivial.hpp>

namespace framework {

lua_bot_t::lua_bot_t(const bot_config_t& bot_config, const lua_bot_config_t& lua_bot_config)
    : bot_config_{bot_config}
    , lua_bot_config_{lua_bot_config}
    , lua_state_{nullptr, lua_close}
{
}

// bot_t
void lua_bot_t::reload()
{
    if(resume_) {
        if(script_path_ == lua_bot_config_.script_path)
            return;
    }

    script_path_ = lua_bot_config_.script_path;

    resume_ = nullptr;
    lua_state_.reset(luaL_newstate());
    if(!lua_state_) throw std::runtime_error{"Cannot create Lua state"};

    luaL_openlibs(lua_state_.get());
    if(int ec = luaL_loadfile(lua_state_.get(), script_path_.c_str())) {
        throw std::runtime_error{"Cannot load Lua script file " + script_path_.string() +
                                 ": " + lua_tostring(lua_state_.get(), -1)};
    }

    BOOST_LOG_TRIVIAL(debug) << "Loaded Lua script";
}

void lua_bot_t::stop()
{

}

} // namespace framework
