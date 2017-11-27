/*
 *  framework/bot.h
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 27/11/2017.
 *
 */

#ifndef FRAMEWORK_BOT_H
#define FRAMEWORK_BOT_H

#include <boost/asio/io_service.hpp>

#include <nlohmann/json.hpp>

#include "framework/config.h"

namespace framework {

class bot_t
{
public:
    bot_t(boost::asio::io_service& io_service,
          const bot_config_t& bot_config);

    void new_update(nlohmann::json update);

    void reload();
    void stop(std::function<void()> cb);

private:
    boost::asio::io_service& io_service_;
    const bot_config_t& bot_config_;
};

} // namespace framework

#endif // FRAMEWORK_BOT_H
