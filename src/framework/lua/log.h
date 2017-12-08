/*
 *  framework/lua/log.h
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 08/12/2017.
 *
 */

#ifndef FRAMEWORK_LUA_LOG_H
#define FRAMEWORK_LUA_LOG_H

#include <lua.hpp>

namespace framework {
namespace lua {

int push_log_table(lua_State* lua_state);

} // namespace lua
} // namespace framework

#endif // FRAMEWORK_LUA_LOG_H
