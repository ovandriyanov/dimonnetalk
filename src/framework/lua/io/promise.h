/*
 *  framework/lua/io/promise.h
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 08/12/2017.
 *
 */

#ifndef FRAMEWORK_LUA_IO_PROMISE_H
#define FRAMEWORK_LUA_IO_PROMISE_H

#include <boost/coroutine2/all.hpp>
#include <boost/optional.hpp>

#include <lua.hpp>

namespace framework {
namespace lua {
namespace io {

class promise_t
{
public:
    static int create(lua_State* lua_state);
    static int destroy(lua_State* lua_state);
    static int promise_get_value(lua_State* lua_state);

    static const char* metatable_name;

private:
    boost::optional<int> value_ref;
    boost::coroutines2::coroutine<void>::push_type* resume;
};

} // namespace io
} // namespace lua
} // namespace framework

#endif // FRAMEWORK_LUA_IO_PROMISE_H
