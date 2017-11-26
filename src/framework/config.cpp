/*
 *  framework/config.cpp
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 23/11/2017.
 *
 */

#include <nlohmann/json.hpp>

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

    auto& update_source_json = json_config.at("update_source");
    std::string update_source_type = update_source_json.at("type");
    if(update_source_type == "longpoll") {
        longpoll_config_t longpoll_config;
        longpoll_config.host = update_source_json.at("host");
        longpoll_config.port = update_source_json.at("port");
        longpoll_config.retry_timeout = update_source_json.at("retry_timeout");
        config.update_source = std::move(longpoll_config);
    } else {
        throw load_error("Invalid update source type: " + update_source_type);
    }

    for(auto& item : json_config.at("bots")) {
        config.bots.emplace_back();
        auto& bot_config = config.bots.back();
        bot_config.name = item.at("name").get<std::string>();
        bot_config.api_token = item.at("api_token").get<std::string>();
        bot_config.script_path = item.at("script_path").get<std::string>();
    }
    return config;
}

} // namespace framework
