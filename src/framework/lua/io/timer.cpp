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

int timer_t::create(lua_State* lua_state)
{
    push_userdata<timer_t>(
        lua_state, "dimonnetalk.io.timer", get_from_registry<boost::asio::io_service>(lua_state, "io_service"));
    return 1;
};

int timer_t::destroy(lua_State* lua_state)
{
    auto& self = to_userdata<timer_t>(lua_state, -1);
    for(int ref : self.callback_refs)
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
    timer_t* timer;
    if(lua_gettop(lua_state) == 2) { // promise overload
        luaL_checkudata(lua_state, -2, metatable_name);
        timer = &to_userdata<timer_t>(lua_state, -2);
        luaL_checkudata(lua_state, -1, promise_t::metatable_name);

        timer->callback_refs.emplace_back(luaL_ref(lua_state, LUA_REGISTRYINDEX));
        timer->timer.async_wait(timer->wrap(
            [=, cbit = std::prev(timer->callback_refs.end())](const error_code& ec)
        {
            if(ec) throw system_error{ec, "timer"};

            int cb_ref = *cbit;
            timer->callback_refs.erase(cbit);

            auto* thread = lua_newthread(lua_state);
            lua_pop(lua_state, 1); // TODO: save reference to this thread
            lua_rawgeti(thread, LUA_REGISTRYINDEX, cb_ref);
            lua_call(thread, 0, 0);
        }));
    } else { // coroutine overload
        luaL_checkudata(lua_state, -1, "dimonnetalk.io.timer");
        timer = &to_userdata<timer_t>(lua_state, -1);
        auto& bot = get_from_registry<bot_t>(lua_state, "bot");
        auto& yield = get_from_registry<boost::coroutines2::coroutine<void>::pull_type>(lua_state, "yield");

        util::async_wait_timer(timer->timer, yield, *bot.resume_);
    }

    return 0;
};

const char* timer_t::metatable_name = "dimonnetalk.io.timer";

} // namespace io
} // namespace lua
} // namespace framework
