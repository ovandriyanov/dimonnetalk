/*
 *  framework/lua/io.cpp
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 08/12/2017.
 *
 */

#include "framework/lua/io.h"
#include "framework/lua/io/timer.h"
#include "util/callback_wrapper.h"

namespace framework {
namespace lua {

int push_io_table(lua_State* lua_state)
{
    luaL_Reg timer_methods[] = {
        {"expires_from_now", io::timer_t::expires_from_now},
        {"async_wait", io::timer_t::async_wait},
        // {"cancel", timer_cancel},
        {"__gc", io::timer_t::destroy},
        {nullptr, nullptr}
    };

    luaL_Reg timer_functions[] = {
        {"new", io::timer_t::create},
        {nullptr, nullptr}
    };

    int exists = !luaL_newmetatable(lua_state, "dimonnetalk.io.timer");
    assert(!exists);
    lua_pushvalue(lua_state, -1);
    lua_setfield(lua_state, -2, "__index");
    luaL_setfuncs(lua_state, timer_methods, 0);
    lua_pop(lua_state, 1);

    lua_newtable(lua_state);

    luaL_newlib(lua_state, timer_functions);
    lua_setfield(lua_state, -2, "timer");

    return 1;
}

} // namespace lua
} // namespace framework
