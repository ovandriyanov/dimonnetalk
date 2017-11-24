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
    service_t(boost::asio::io_service& io_service) : io_service_{io_service} {}

    virtual ~service_t() = default;
    virtual void reload() {}
    virtual void stop(std::function<void()> cb) { io_service_.post(std::move(cb)); }

protected:
    boost::asio::io_service& io_service_;
};

} // namespace framework

#endif // FRAMEWORK_SERVICE_H
