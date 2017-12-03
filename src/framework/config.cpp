/*
 *  framework/config.cpp
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 23/11/2017.
 *
 */

#include "framework/config.h"
#include "util/file_chunk_range.h"
#include "util/flatten_range.h"

namespace framework {

namespace fs = boost::filesystem;

config_t load_config(const fs::path& path)
{
    auto load_error = [&](const std::string& what) {
        return std::runtime_error{"Cannot load configuration file " + path.string() + ": " + what};
    };

    util::file_chunk_range_t chunk_range{path};
    util::flatten_range_t<char> flatten_range{chunk_range};
    std::string config_str{flatten_range.begin(), flatten_range.end()};
    auto json_config = nlohmann::json::parse(config_str);

    config_t config;

    auto& api_server_json = json_config.at("api_server");
    config.api_server.host = api_server_json.at("host");
    if(config.api_server.host.empty())
        throw std::runtime_error{"API server host must not be empty"};
    config.api_server.port = api_server_json.at("port");

    auto& update_source_json = json_config.at("update_source");
    std::string update_source_type = update_source_json.at("type");
    if(update_source_type == "longpoll") {
        longpoll_config_t longpoll_config;
        longpoll_config.retry_timeout = update_source_json.at("retry_timeout");
        longpoll_config.poll_timeout = update_source_json.at("poll_timeout");
        config.update_source = std::move(longpoll_config);
    } else {
        throw load_error("Invalid update source type: " + update_source_type);
    }

    for(auto& item : json_config.at("bots")) {
        config.bots.emplace_back();
        auto& bot_config = config.bots.back();
        bot_config.type = item.at("type").get<std::string>();
        bot_config.api_token = item.at("api_token").get<std::string>();
        if(bot_config.type == "lua") {
            lua_bot_config_t lua_bot_config;
            lua_bot_config.script_path = item.at("script_path").get<std::string>();
            bot_config.type_specific_config = std::move(lua_bot_config);
            auto bot_data = item.find("bot_data");
            if(bot_data != item.end())
                bot_config.bot_data = std::move(*bot_data);
        } else {
            throw load_error("Unsupported bot type: " + bot_config.type);
        }
    }
    return config;
}

} // namespace framework
