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
    auto parse_error = [&](const std::string& what) {
        return std::runtime_error("Cannot parse configuration file " + path.string() + ": " + what);
    };

    try {
        util::file_chunk_range_t chunk_range{path};
        util::flatten_range_t<char> flatten_range{chunk_range};
        std::string config_str{flatten_range.begin(), flatten_range.end()};
        auto json_config = nlohmann::json::parse(config_str);

        config_t config;
        auto& event_source_json = json_config.at("event_source");
        const std::string& type = event_source_json.at("type");
        if(type == "longpoll") {
            longpoll_config_t longpoll_config;
            longpoll_config.host = event_source_json.at("host");
            if(longpoll_config.host.empty())
                throw parse_error("longpoll event source config: empty host name is invalid");
            config.event_source_config = std::move(longpoll_config);
        } else {
            throw parse_error("invalid event source type: " + type);
        }

        for(auto& item : json_config.at("bots")) {
            config.bots.emplace_back();
            auto& bot_config = config.bots.back();
            bot_config.name = item.at("name").get<std::string>();
            bot_config.api_token = item.at("api_token").get<std::string>();
            bot_config.script_path = item.at("script_path").get<std::string>();
        }
        return config;
    } catch(std::exception& ex) {
        throw parse_error(ex.what());
    }
}

} // namespace framework
