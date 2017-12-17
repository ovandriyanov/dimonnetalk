/*
 *  framework/lua/globals.h
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 08/12/2017.
 *
 */

#ifndef FRAMEWORK_LUA_GLOBALS_H
#define FRAMEWORK_LUA_GLOBALS_H

#include <boost/asio/io_service.hpp>

#include "framework/lua/bot.h"
#include "util/coroutine.h"

namespace framework {
namespace lua {

struct globals_t
{
    bot_t* bot;
};

extern const char* globals_name;

} // namespace framework
} // namespace lua

#endif // FRAMEWORK_LUA_GLOBALS_H
