/*
 *  framework/bot_store.h
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 27/11/2017.
 *
 */

#ifndef FRAMEWORK_BOT_STORE_H
#define FRAMEWORK_BOT_STORE_H

#include "framework/api_server.h"
#include "framework/bot.h"
#include "framework/config.h"
#include "framework/service.h"

namespace framework {

class bot_store_t : public service_t
{
public:
    bot_store_t(boost::asio::io_service& io_service, const config_t& config, api_server_t& api_server);

private: // service_t
    void reload() final;
    void stop(std::function<void()> cb) final;

private:
    const config_t& config_;
    api_server_t& api_server_;

    std::map<std::tuple<std::string /* API token */, std::string /* type */>, std::unique_ptr<bot_t>> bots_;
};

} // namespace framework

#endif // FRAMEWORK_BOT_STORE_H
