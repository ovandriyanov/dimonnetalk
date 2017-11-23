/*
 *  util/file_chunk_range.h
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 23/11/2017.
 *
 */

#ifndef UTIL_FILE_CHUNK_RANGE_H
#define UTIL_FILE_CHUNK_RANGE_H

#include <boost/asio/buffer.hpp>
#include <boost/coroutine2/all.hpp>
#include <boost/coroutine2/detail/pull_coroutine.hpp>
#include <boost/filesystem/path.hpp>

namespace util {

class file_chunk_range_t
{
public:
    using coro_t = boost::coroutines2::coroutine<boost::asio::const_buffer>;
    using iterator = coro_t::pull_type::iterator;

public:
    file_chunk_range_t(const boost::filesystem::path& path);

    auto begin() { return boost::coroutines2::detail::begin(chunk_source_); }
    auto end() { return boost::coroutines2::detail::end(chunk_source_); }

private:
    coro_t::pull_type chunk_source_;
};

} // namespace util

#endif // UTIL_FILE_CHUNK_RANGE_H
