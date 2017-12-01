/*
 *  util/coroutine.h
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 29/11/2017.
 *
 */

#ifndef UTIL_COROUTINE_H
#define UTIL_COROUTINE_H

#include <boost/coroutine2/all.hpp>

#include "util/callback_wrapper.h"

namespace util {

class push_coro_t : public callback_wrapper_t
                  , public boost::coroutines2::coroutine<void>::push_type
{
public:
    template <typename F>
    push_coro_t(F&& function) : boost::coroutines2::coroutine<void>::push_type{std::forward<F>(function)} {}
};

class pull_coro_t : public callback_wrapper_t
                  , public boost::coroutines2::coroutine<void>::pull_type
{
public:
    template <typename F>
    pull_coro_t(F&& function) : boost::coroutines2::coroutine<void>::pull_type{std::forward<F>(function)} {}
};

};

#endif // UTIL_COROUTINE_H
