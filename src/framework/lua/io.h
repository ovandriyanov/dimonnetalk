/*
 *  framework/lua/io.h
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 08/12/2017.
 *
 */

#ifndef FRAMEWORK_LUA_IO_H
#define FRAMEWORK_LUA_IO_H

#include <lua.hpp>

namespace framework {
namespace lua {

int push_io_table(lua_State* lua_state);

} // namespace framework
} // namespace lua

#endif // FRAMEWORK_LUA_IO_H
