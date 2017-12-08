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

#include "util/coroutine.h"

namespace framework {
namespace lua {

struct globals_t
{
    boost::asio::io_service& io_service;
    boost::coroutines2::coroutine<void>::pull_type& yield;
    util::push_coro_t& resume;
};

extern const char* globals_name;

} // namespace framework
} // namespace lua

#endif // FRAMEWORK_LUA_GLOBALS_H
