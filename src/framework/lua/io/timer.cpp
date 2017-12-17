/*
 *  framework/lua/io/timer.cpp
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 08/12/2017.
 *
 */

#include "framework/lua/io/promise.h"
#include "framework/lua/io/timer.h"

namespace framework {
namespace lua {
namespace io {

using namespace boost::system;

int timer_t::create(lua_State* lua_state)
{
    push_userdata<timer_t>(
        lua_state, "dimonnetalk.io.timer", get_from_registry<boost::asio::io_service>(lua_state, "io_service"));
    return 1;
};

int timer_t::destroy(lua_State* lua_state)
{
    auto& self = to_userdata<timer_t>(lua_state, -1);
    for(int ref : self.thread_refs)
        luaL_unref(lua_state, LUA_REGISTRYINDEX, ref);
    self.~timer_t();
    lua_pop(lua_state, 1);
    return 0;
}

int timer_t::expires_from_now(lua_State* lua_state)
{
    luaL_checkudata(lua_state, -2, metatable_name);
    auto& timer = to_userdata<timer_t>(lua_state, -2);
    auto ms = luaL_checkinteger(lua_state, -1);
    lua_pop(lua_state, 2);

    timer.timer.expires_after(std::chrono::milliseconds{ms});
    return 0;
};

int timer_t::async_wait(lua_State* lua_state)
{
    luaL_checkudata(lua_state, -1, "dimonnetalk.io.timer");
    timer_t& self = to_userdata<timer_t>(lua_state, -1);

    lua_pushthread(lua_state);
    self.thread_refs.emplace_back(luaL_ref(lua_state, LUA_REGISTRYINDEX));

    self.timer.async_wait(self.timer.wrap(
        [&self, lua_state, ref = std::prev(self.thread_refs.end())](error_code ec)
    {
        luaL_unref(lua_state, LUA_REGISTRYINDEX, *ref);
        self.thread_refs.erase(ref);

        if(ec) throw system_error{ec, "timer"};
        lua_resume(lua_state, NULL, 0);
    }));

    return lua_yield(lua_state, 0);
};

const char* timer_t::metatable_name = "dimonnetalk.io.timer";

} // namespace io
} // namespace lua
} // namespace framework
