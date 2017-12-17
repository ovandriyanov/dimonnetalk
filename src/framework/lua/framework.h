/*
 *  framework/lua/framework.h
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 17/12/2017.
 *
 */

#ifndef FRAMEWORK_LUA_FRAMEWORK_H
#define FRAMEWORK_LUA_FRAMEWORK_H

#include <lua.hpp>

#include "framework/api_server.h"
#include "framework/longpoll_update_source.h"

namespace framework {
namespace lua {

int push_framework_table(lua_State* lua_state);
void fill_framework_table(lua_State* lua_state, api_server_t* api_server, longpoll_update_source_t* update_source);

} // namespace lua
} // namespace framework

#endif // FRAMEWORK_LUA_FRAMEWORK_H
