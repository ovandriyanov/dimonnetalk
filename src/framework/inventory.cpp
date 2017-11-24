/*
 *  framework/inventory.cpp
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 19/11/2017.
 *
 */

#include <boost/range/adaptor/reversed.hpp>

#include "framework/config.h"
#include "framework/inventory.h"

#include <boost/log/trivial.hpp>

namespace framework {

namespace fs = boost::filesystem;

namespace {

class config_service_t : public service_t
{
public:
    config_service_t(boost::asio::io_service& io_service, fs::path config_path)
        : service_t{io_service}
        , config_path_{std::move(config_path)} {}

    config_t& config() { return config_; }

public: // service_t
    void reload() final
    {
        config_ = load_config(config_path_);
        BOOST_LOG_TRIVIAL(debug) << "Loaded configuration file " << config_path_.string();
    }

private:
    fs::path config_path_;
    config_t config_;
};

} // unnamed namespace

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

    stop_coro_.emplace([&, cb = std::move(cb)](coro_t::push_type& yield) {
        for(auto& s : services_ | reversed)
            s->stop([&]() { (*stop_coro_)(); });
        for(int i = services_.size(); i; --i)
            yield();
        services_.clear();
        io_service.post(std::move(cb));
    });
}

inventory_t make_inventory(boost::asio::io_service& io_service, fs::path config_path)
{
    auto config_service = std::make_unique<config_service_t>(io_service, config_path);

    std::list<std::unique_ptr<service_t>> services;
    services.emplace_back(std::move(config_service));

    return inventory_t{std::move(services)};
}

} // namespace framework
