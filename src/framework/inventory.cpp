/*
 *  framework/inventory.cpp
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 19/11/2017.
 *
 */

#include <boost/range/adaptor/reversed.hpp>
#include <boost/coroutine2/all.hpp>

#include "framework/inventory.h"

namespace framework {

inventory_t::inventory_t(std::list<std::unique_ptr<service_t>> services)
    : services_{std::move(services)}
{
}

void inventory_t::reload()
{
    for(auto& s : services_) s->reload();
}

void inventory_t::stop(boost::asio::io_service& io_service, std::function<void()> cb)
{
    using namespace boost::adaptors;
    using coro_t = boost::coroutines2::coroutine<void>;
    coro_t::pull_type resume{[&, cb = std::move(cb)](coro_t::push_type& yield) {
        for(auto& s : services_ | reversed)
            s->stop(io_service, [&]() { resume(); });
        for(int i = services_.size(); i; --i)
            yield();
        services_.clear();
        io_service.post(std::move(cb));
    }};
}

inventory_t make_inventory()
{
    return inventory_t{{}};
}

} // namespace framework
