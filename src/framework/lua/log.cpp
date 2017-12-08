/*
 *  framework/lua/log.cpp
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 08/12/2017.
 *
 */

#include <boost/log/trivial.hpp>

#include "framework/lua/log.h"

namespace framework {
namespace lua {

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

int push_log_table(lua_State* lua_state)
{
    lua_newtable(lua_state);
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
        lua_pushinteger(lua_state, std::get<0>(level));
        lua_pushcclosure(lua_state, log, 1);
        lua_setfield(lua_state, -2, std::get<1>(level));
    }
    return 1;
}

} // namespace lua
} // namespace framework
