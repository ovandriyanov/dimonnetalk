/*
 *  util/flatten_range.h
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 24/11/2017.
 *
 */

#ifndef UTIL_FLATTEN_RANGE_H
#define UTIL_FLATTEN_RANGE_H

#include <boost/coroutine2/all.hpp>
#include <boost/coroutine2/detail/pull_coroutine.hpp>

#include "util/asio.h"

namespace util {

template <typename Item>
class flatten_range_t
{
public:
    using coro_t = boost::coroutines2::coroutine<Item>;

public:
    template <typename Range>
    flatten_range_t(Range& range)
        : item_source_{[&](typename coro_t::push_type& yield) mutable
    {
        for(auto&& item : range)
            for(auto&& nested : item)
                yield(std::forward<decltype(nested)>(nested));
    }}
    {
    }

    auto begin() { return boost::coroutines2::detail::begin(item_source_); }
    auto end() { return boost::coroutines2::detail::end(item_source_); }

private:
    typename coro_t::pull_type item_source_;
};

} // namespace util

#endif // UTIL_FLATTEN_RANGE_H
