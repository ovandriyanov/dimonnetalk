/*
 *  framework/lua/io/promise.cpp
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 08/12/2017.
 *
 */

#include "framework/lua/io/promise.h"
#include "framework/lua/globals.h"
#include "framework/lua/util.h"

namespace framework {
namespace lua {
namespace io {

int promise_t::create(lua_State* lua_state)
{
    push_userdata<promise_t>(lua_state, metatable_name);
    return 1;
};

int promise_t::destroy(lua_State* lua_state)
{
    luaL_checkudata(lua_state, -1, "dimonnetalk.io.promise");
    auto& promise = to_userdata<promise_t>(lua_state, -1);
    if(promise.value_ref)
        luaL_unref(lua_state, LUA_REGISTRYINDEX, *promise.value_ref);
    promise.~promise_t();
    return 0;
};

int promise_t::promise_get_value(lua_State* lua_state)
{
    luaL_checkudata(lua_state, -1, "dimonnetalk.io.promise");
    auto& promise = to_userdata<promise_t>(lua_state, -1);

    if(!promise.value_ref) {
        // wait for value
        assert(!promise.resume);
        auto& globals = get_from_registry<globals_t>(lua_state, globals_name);
        promise.resume = &globals.resume;
        globals.yield();
    }
    assert(promise.value_ref);
    promise.resume = nullptr;
    lua_rawgeti(lua_state, LUA_REGISTRYINDEX, *promise.value_ref);
    return 1;
};

const char* metatable_name = "dimonnetalk.io.promise";

} // namespace io
} // namespace lua
} // namespace framework
