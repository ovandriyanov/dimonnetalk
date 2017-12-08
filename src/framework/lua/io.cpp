/*
 *  framework/lua/io.cpp
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 08/12/2017.
 *
 */

#include "framework/lua/io.h"
#include "util/callback_wrapper.h"

namespace framework {
namespace lua {

int push_io_table(lua_State* lua_state)
{
    luaL_Reg timer_methods[] = {
        {"expires_from_now", timer_expires_from_now},
        {"async_wait", timer_async_wait},
        // {"cancel", timer_cancel},
        {"__gc", destroy_userdata<timer_t>},
        {nullptr, nullptr}
    };

    luaL_Reg timer_functions[] = {
        {"new", timer_new},
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

    class promise_t
    {
    public:
        promise_t() : resume{nullptr} {}

    public:
        boost::optional<int> value_ref;
        boost::coroutines2::coroutine<void>::push_type* resume;
    };

    auto promise_set_value = [](lua_State* lua_state) {
        luaL_checkudata(lua_state, -2, "dimonnetalk.io.promise");
        auto& promise = to_userdata<promise_t>(lua_state, -2);
        luaL_checkany(lua_state, -1);

        if(promise.value_ref) return 0; // value is already set
        promise.value_ref = luaL_ref(lua_state, LUA_REGISTRYINDEX);
        if(promise.resume) { // waiting for value
            auto* resume = promise.resume;
            promise.resume = nullptr;
            (*resume)();
        }
        return 0;
    };

    auto promise_get_value = [](lua_State* lua_state) {
        luaL_checkudata(lua_state, -1, "dimonnetalk.io.promise");
        auto& promise = to_userdata<promise_t>(lua_state, -1);

        if(!promise.value_ref) {
            // wait for value
            assert(!promise.resume);
            auto& bot = get_from_registry<lua_bot_t>(lua_state, "bot");
            promise.resume = bot.resume_.get();
            auto& yield = get_from_registry<boost::coroutines2::coroutine<void>::push_type>(lua_state, "yield");
            yield();
        }
        assert(promise.value_ref);
        promise.resume = nullptr;
        lua_rawgeti(lua_state, LUA_REGISTRYINDEX, *promise.value_ref);
        return 1;
    };

    auto promise_destroy = [](lua_State* lua_state) {
        luaL_checkudata(lua_state, -1, "dimonnetalk.io.promise");
        auto& promise = to_userdata<promise_t>(lua_state, -1);
        if(promise.value_ref)
            luaL_unref(lua_state, LUA_REGISTRYINDEX, *promise.value_ref);
        promise.~promise_t();
        return 0;
    };

    auto promise_new = [](lua_State* lua_state) {
        push_userdata<promise_t>(lua_state, "dimonnetalk.io.promise");
        return 1;
    };

    luaL_Reg promise_methods[] = {
        {"get_value", promise_get_value},
        {"set_value", promise_set_value},
        {"__gc", promise_destroy},
        {"__gc", promise_destroy},
        {nullptr, nullptr}
    };

    luaL_Reg promise_functions[] = {
        {"new", promise_new},
        {nullptr, nullptr}
    };

    exists = !luaL_newmetatable(lua_state, "dimonnetalk.io.promise");
    assert(!exists);
    lua_pushvalue(lua_state, -1);
    lua_setfield(lua_state, -2, "__index");
    luaL_setfuncs(lua_state, promise_methods, 0);
    lua_pop(lua_state, 1);

    luaL_newlib(lua_state, promise_functions);
    lua_setfield(lua_state, -2, "promise");

    return 1;
}

} // namespace lua
} // namespace framework
