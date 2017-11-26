/*
 *  util/file_chunk_range.cpp
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 23/11/2017.
 *
 */

#include <boost/filesystem/fstream.hpp>

#include "util/file_chunk_range.h"

namespace util {

namespace fs = boost::filesystem;
using namespace boost::system;

file_chunk_range_t::file_chunk_range_t(const boost::filesystem::path& path)
    : chunk_source_{[=](coro_t::push_type& yield)
{
    fs::ifstream is{path};
    if(!is) throw system_error{error_code{errno, system_category()}, "Cannot open " + path.string()};

    char buf[BUFSIZ];
    while(is.read(buf, sizeof(buf)))
        yield(boost::asio::buffer(buf, is.gcount()));

    if(is.bad()) throw system_error{error_code(errno, system_category()), "Cannot read " + path.string()};

    if(is.gcount())
        yield(boost::asio::buffer(buf, is.gcount()));
}}
{
}

} // namespace util
