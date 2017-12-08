/*
 *  framework/lua/io/timer.h
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 08/12/2017.
 *
 */

#ifndef FRAMEWORK_LUA_IO_TIMER_H
#define FRAMEWORK_LUA_IO_TIMER_H

#include <list>

#include <boost/asio/io_service.hpp>
#include <boost/asio/steady_timer.hpp>

#include <lua.hpp>

#include "util/callback_wrapper.h"
#include "framework/lua/util.h"

namespace framework {
namespace lua {
namespace io {

class timer_t : public util::callback_wrapper_t
{
public:
    timer_t(boost::asio::io_service& io_service) : timer{io_service} {}

    static int create(lua_State* lua_state);
    static int destroy(lua_State* lua_state);

    static int expires_from_now(lua_State* lua_state);
    static int async_wait(lua_State* lua_state);

    static const char* metatable_name;

public:
    boost::asio::steady_timer timer;
    std::list<int> callback_refs;
};

} // namespace io
} // namespace lua
} // namespace framework

#endif // FRAMEWORK_LUA_IO_TIMER_H
