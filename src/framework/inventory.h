/*
 *  framework/inventory.h
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 19/11/2017.
 *
 */

#ifndef FRAMEWORK_INVENTORY_H
#define FRAMEWORK_INVENTORY_H

#include <list>
#include <memory>

#include <boost/coroutine2/all.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/optional.hpp>

#include "framework/service.h"

namespace framework {

class inventory_t
{
public:
    inventory_t(std::list<std::unique_ptr<service_t>> services);

    void reload();
    void stop(boost::asio::io_service& io_service, std::function<void()> cb);

private:
    using coro_t = boost::coroutines2::coroutine<void>;

    std::list<std::unique_ptr<service_t>> services_;
    boost::optional<coro_t::pull_type> stop_coro_;
};

inventory_t make_inventory(boost::asio::io_service& io_service, boost::filesystem::path config_path);

} // namespace framework

#endif // FRAMEWORK_INVENTORY_H
