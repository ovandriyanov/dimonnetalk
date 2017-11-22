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

#include "framework/service.h"

namespace framework {

class inventory_t
{
public:
    inventory_t(std::list<std::unique_ptr<service_t>> services);

    void reload();
    void stop(boost::asio::io_service& io_service, std::function<void()> cb);

private:
    std::list<std::unique_ptr<service_t>> services_;
};

inventory_t make_inventory();

} // namespace framework

#endif // FRAMEWORK_INVENTORY_H
