/*
 *  framework/service.h
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 19/11/2017.
 *
 */

#ifndef FRAMEWORK_SERVICE_H
#define FRAMEWORK_SERVICE_H

#include <functional>

#include <boost/asio/io_service.hpp>

namespace framework {

class service_t
{
public:
    virtual ~service_t() = default;
    virtual void reload() {}
    virtual void stop(boost::asio::io_service& io_service, std::function<void()> cb) { io_service.post(std::move(cb)); }
};

} // namespace framework

#endif // FRAMEWORK_SERVICE_H
