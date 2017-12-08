/*
 *  framework/lua/util.h
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 08/12/2017.
 *
 */

#ifndef FRAMEWORK_LUA_UTIL_H
#define FRAMEWORK_LUA_UTIL_H

#include <cassert>
#include <utility>

#include <lua.hpp>

namespace framework {
namespace lua {

template <typename T>
static T& to_userdata(lua_State* lua_state, int pos)
{
    assert(lua_islightuserdata(lua_state, pos) || lua_isuserdata(lua_state, pos));
    auto& ret = *static_cast<T*>(lua_touserdata(lua_state, pos));
    return ret;
}

template <typename T, typename... Args>
static void push_userdata(lua_State* lua_state, const char* metatable_name, Args&&... args)
{
    void* ptr = lua_newuserdata(lua_state, sizeof(T));
    auto* timer = new(ptr) T{std::forward<Args>(args)...};
    luaL_setmetatable(lua_state, metatable_name);
}

template <typename T>
static T& get_from_registry(lua_State* lua_state, const char* name)
{
    lua_getfield(lua_state, LUA_REGISTRYINDEX, name);
    auto& ret = to_userdata<T>(lua_state, -1);
    lua_pop(lua_state, 1);
    return ret;
}

} // namespace lua
} // namespace framework

#endif // FRAMEWORK_LUA_UTIL_H
