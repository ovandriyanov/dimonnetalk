/*
 *  framework/update_source_switch.h
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 26/11/2017.
 *
 */

#ifndef FRAMEWORK_UPDATE_SOURCE_SWITCH_H
#define FRAMEWORK_UPDATE_SOURCE_SWITCH_H

#include <boost/optional.hpp>

#include "framework/config.h"
#include "framework/longpoll_update_source.h"
#include "framework/service.h"

namespace framework {

class update_source_switch_t : public service_t
{
public:
    update_source_switch_t(boost::asio::io_service& io_service, const config_t& config);

public: // service_t
    void reload() final;
    void stop(std::function<void()> cb) final;

private:
    using longpoll_update_sources_t = std::list<std::unique_ptr<longpoll_update_source_t>>;
    using update_source_t = boost::variant<longpoll_update_sources_t>;

    void stop(longpoll_update_sources_t& longpoll_sources);
    void stop(update_source_t& variant_source);

private:
    const config_t& config_;

    boost::optional<update_source_t> update_source_;
    std::list<update_source_t> stopping_;
    std::function<void()> stop_cb_;
};

} // namespace framework

#endif // FRAMEWORK_UPDATE_SOURCE_SWITCH_H
