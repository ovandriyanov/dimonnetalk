/*
 *  framework/bot_store.cpp
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 27/11/2017.
 *
 */

#include "framework/bot_store.h"
#include "framework/lua_bot.h"

namespace framework {

bot_store_t::bot_store_t(boost::asio::io_service& io_service, const config_t& config)
    : service_t{io_service}
    , config_{config}
{
}

// service_t
void bot_store_t::reload()
{
    struct visitor_t : public boost::static_visitor<std::unique_ptr<bot_t>>
    {
        visitor_t(bot_store_t& bot_store, const bot_config_t& bot_config) : bot_store{bot_store}, bot_config{bot_config} {}

        std::unique_ptr<bot_t> operator()(const lua_bot_config_t& lua_bot_config)
        {
            return std::make_unique<lua_bot_t>(bot_config, lua_bot_config);
        }

        bot_store_t& bot_store;
        const bot_config_t& bot_config;
    };

    std::map<std::tuple<std::string, std::string>, std::unique_ptr<bot_t>> to_reload;
    for(const auto& bot_config : config_.bots) {
        auto key = std::make_tuple(bot_config.api_token, bot_config.type);
        auto found = bots_.find(key);
        if(found == bots_.end()) {
            visitor_t visitor{*this, bot_config};
            to_reload.emplace(std::move(key), boost::apply_visitor(visitor, bot_config.type_specific_config));
        } else {
            to_reload.emplace(std::move(*found));
            bots_.erase(found);
        }
    }

    for(auto& bot : bots_)
        bot.second->stop();

    bots_ = std::move(to_reload);
    for(auto& bot : bots_)
        bot.second->reload();
}

void bot_store_t::stop(std::function<void()> cb)
{
    for(auto& bot : bots_)
        bot.second->stop();
    io_service_.post(std::move(cb));
}

} // namespace framework
