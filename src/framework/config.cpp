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

config_t load_config(const fs::path& path) try
{
    util::file_chunk_range_t chunk_range{path};
    util::flatten_range_t<char> flatten_range{chunk_range};
    std::string config_str{flatten_range.begin(), flatten_range.end()};
    auto json_config = nlohmann::json::parse(config_str);

    config_t config;
    for(auto& item : json_config.at("bots")) {
        config.bots.emplace_back();
        auto& bot_config = config.bots.back();
        bot_config.name = item.at("name").get<std::string>();
        bot_config.api_token = item.at("api_token").get<std::string>();
        bot_config.script_path = item.at("script_path").get<std::string>();
    }
    return config;
} catch(nlohmann::json::parse_error& ex) {
    throw std::runtime_error{"Cannot parse configuration file " + path.string() + ": " + ex.what()};
}

} // namespace framework
