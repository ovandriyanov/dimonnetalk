/*
 *  framework/config.h
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 23/11/2017.
 *
 */

#ifndef FRAMEWORK_CONFIG_H
#define FRAMEWORK_CONFIG_H

#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/variant.hpp>

#include <nlohmann/json.hpp>

namespace framework {

struct api_server_config_t
{
    std::string host;
    uint16_t port;
};

struct longpoll_config_t
{
    int retry_timeout;
    int poll_timeout;
};

using update_source_config_t = boost::variant<longpoll_config_t>;

struct lua_bot_config_t
{
    boost::filesystem::path script_path;
};

struct bot_config_t
{
    std::string type;
    std::string api_token;
    boost::variant<lua_bot_config_t> type_specific_config;
    nlohmann::json bot_data;
};

struct config_t
{
    api_server_config_t api_server;
    update_source_config_t update_source;
    std::vector<bot_config_t> bots;
};

config_t load_config(const boost::filesystem::path& path);

} // namespace framework

#endif // FRAMEWORK_CONFIG_H
