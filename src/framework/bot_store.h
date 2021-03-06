/*
 *  framework/bot_store.h
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 27/11/2017.
 *
 */

#ifndef FRAMEWORK_BOT_STORE_H
#define FRAMEWORK_BOT_STORE_H

#include "framework/bot.h"
#include "framework/config.h"
#include "framework/service.h"

namespace framework {

class bot_store_t : public service_t
{
public:
    bot_store_t(boost::asio::io_service& io_service, const config_t& config);

private: // service_t
    void reload() final;
    void stop(std::function<void()> cb) final;

private:
    const config_t& config_;

    std::map<std::string /* API token */, std::shared_ptr<bot_t>> bots_;
};

} // namespace framework

#endif // FRAMEWORK_BOT_STORE_H
