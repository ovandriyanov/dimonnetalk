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

static int log(lua_State* lua_state)
{
    #define LOG(LEVEL)                                                                             \
    BOOST_LOG_STREAM_WITH_PARAMS(                                                                  \
        boost::log::trivial::logger::get(),                                                        \
        (boost::log::keywords::severity = static_cast<boost::log::trivial::severity_level>(LEVEL)) \
    )

    if(lua_gettop(lua_state) != 1) {
        lua_pushstring(lua_state, "Log function receives exactly one argument");
        lua_error(lua_state);
    }

    if(lua_type(lua_state, -1) != LUA_TSTRING) {
        lua_pushstring(lua_state, "Argument no a log function is not a string");
        lua_error(lua_state);
    }

    size_t len;
    const char* data = lua_tolstring(lua_state, -1, &len);
    std::string s{data, len};

    LOG(lua_tointeger(lua_state, lua_upvalueindex(1))) << s;

    return 0;

    #undef LOG
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

    setup_lua_globals();
    if(lua_pcall(lua_state_.get(), 0, 0, 0)) {
        throw std::runtime_error{"Cannot execute Lua script file " + script_path_.string() +
                                 ": " + lua_tostring(lua_state_.get(), -1)};
    }

    if(lua_getglobal(lua_state_.get(), "start") != LUA_TFUNCTION)
        throw std::runtime_error{"Cannot find function 'start' in file " + script_path_.string()};

    BOOST_LOG_TRIVIAL(debug) << "Loaded Lua script " << script_path_;
}

void lua_bot_t::stop()
{

}

void lua_bot_t::setup_lua_globals()
{
    lua_newtable(lua_state_.get()); // "dimonnetalk" table

    lua_newtable(lua_state_.get()); // "log" table

    std::tuple<boost::log::trivial::severity_level, const char*> levels[] = {
        {boost::log::trivial::trace,   "trace"},
        {boost::log::trivial::debug,   "debug"},
        {boost::log::trivial::info,    "info"},
        {boost::log::trivial::warning, "warning"},
        {boost::log::trivial::error,   "error"},
        {boost::log::trivial::fatal,   "fatal"}
    };

    for(auto level : levels) {
        lua_CFunction x;
        lua_pushinteger(lua_state_.get(), std::get<0>(level));
        lua_pushcclosure(lua_state_.get(), log, 1);
        lua_setfield(lua_state_.get(), -2, std::get<1>(level));
    }
    lua_setfield(lua_state_.get(), -2, "log");

    // lua_newtable(lua_state_.get()); // "timer" table

    lua_setglobal(lua_state_.get(), "dimonnetalk");
}

} // namespace framework
