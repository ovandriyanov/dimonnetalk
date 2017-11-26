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

namespace framework {

struct longpoll_config_t
{
    std::string host;
    uint16_t port;
    int retry_timeout;
};

using update_source_config_t = boost::variant<longpoll_config_t>;

struct bot_config_t
{
    std::string name;
    std::string api_token;
    boost::filesystem::path script_path;
};

struct config_t
{
    std::vector<bot_config_t> bots;
    update_source_config_t update_source;
};

config_t load_config(const boost::filesystem::path& path);

} // namespace framework

#endif // FRAMEWORK_CONFIG_H
