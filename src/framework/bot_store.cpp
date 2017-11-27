/*
 *  framework/bot_store.cpp
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 27/11/2017.
 *
 */

#include "framework/bot_store.h"

namespace framework {

bot_store_t::bot_store_t(boost::asio::io_service& io_service, const config_t& config)
    : service_t{io_service}
    , config_{config}
{
}

// service_t
void bot_store_t::reload()
{
    std::map<std::string, std::shared_ptr<bot_t>> to_reload;
    for(const auto& bot_config : config_.bots) {
        auto found = bots_.find(bot_config.api_token);
        if(found != bots_.end()) {
            to_reload.emplace(std::move(*found));
            bots_.erase(found);
        }
    }

    for(auto& bot : bots_)
        bot.second->stop([ptr = bot.second](){});

    bots_ = std::move(to_reload);
    for(auto& bot : bots_)
        bot.second->reload();
}

void bot_store_t::stop(std::function<void()> cb)
{
    for(auto& bot : bots_)
        bot.second->stop([ptr = bot.second](){});
    io_service_.post(std::move(cb));
}

} // namespace framework
