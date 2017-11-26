/*
 *  framework/update_source_switch.cpp
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 26/11/2017.
 *
 */

#include <boost/log/trivial.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>

#include "framework/update_source_switch.h"

namespace framework {

update_source_switch_t::update_source_switch_t(boost::asio::io_service& io_service, const config_t& config)
    : service_t{io_service}
    , config_{config}
{
}

// service_t
void update_source_switch_t::reload()
{
    struct config_visitor_t : public boost::static_visitor<>
    {
        config_visitor_t(update_source_switch_t& source_switch) : source_switch_{source_switch} {}

        void operator()(const longpoll_config_t& cfg)
        {
            if(!source_switch_.update_source_) {
                // Initial construction
                longpoll_update_sources_t sources;
                for(const auto& bot_config : source_switch_.config_.bots) {
                    sources.emplace_back(std::make_unique<longpoll_update_source_t>(
                        source_switch_.io_service_, cfg, bot_config.api_token,
                        [](nlohmann::json update) { BOOST_LOG_TRIVIAL(debug) << "Got update: \n" << update.dump(4, ' '); }));
                }
                source_switch_.update_source_ = std::move(sources);
            }
            assert(source_switch_.update_source_);

            if(!boost::get<longpoll_update_sources_t>(&*source_switch_.update_source_))
                source_switch_.stop(*source_switch_.update_source_);

            // Reload
            longpoll_update_sources_t stopping;
            auto& list = boost::get<longpoll_update_sources_t>(*source_switch_.update_source_);
            auto it = list.begin();
            auto& bots_config = source_switch_.config_.bots;
            auto get_api_token = [](const bot_config_t& config) -> const std::string& { return config.api_token; };

            while(it != list.end()) {
                // Stop update sources for stopped bots, reload others
                using namespace boost::adaptors;
                auto rg = bots_config | transformed(get_api_token);
                if(boost::find(rg, (*it)->api_token()) == rg.end()) {
                    stopping.emplace_back(std::move(*it));
                    list.erase(it++);
                } else {
                    (*it)->reload();
                    ++it;
                }
            }
            if(stopping.size()) source_switch_.stop(stopping);
        }

        update_source_switch_t& source_switch_;
    } visitor{*this};

    boost::apply_visitor(visitor, config_.update_source);
}

void update_source_switch_t::stop(std::function<void()> cb)
{
    if(!update_source_) return io_service_.post(std::move(cb));

    stop_cb_ = std::move(cb);
    stop(*update_source_);
}

void update_source_switch_t::stop(longpoll_update_sources_t& longpoll_sources)
{
    assert(longpoll_sources.size());

    stopping_.emplace_back(std::move(longpoll_sources));
    auto& list = boost::get<longpoll_update_sources_t>(stopping_.back());
    for(auto it = list.begin(); it != list.end(); ++it) {
        (*it)->stop([this, &list, it, list_it = std::prev(stopping_.end())]() {
            list.erase(it);
            if(list.empty()) {
                stopping_.erase(list_it);
                if(stopping_.empty() && stop_cb_)
                    stop_cb_();
            }
        });
    }
}

void update_source_switch_t::stop(update_source_t& variant_source)
{
    struct source_visitor_t : public boost::static_visitor<>
    {
        source_visitor_t(update_source_switch_t& source_switch) : source_switch_{source_switch} {}

        void operator()(longpoll_update_sources_t& longpoll_sources)
        {
            if(longpoll_sources.size()) {
                auto sources = std::move(longpoll_sources);
                longpoll_sources.clear();
                source_switch_.stop(sources);
            } else {
                source_switch_.io_service_.post(std::move(source_switch_.stop_cb_));
            }
        }

        update_source_switch_t& source_switch_;
    } visitor(*this);

    boost::apply_visitor(visitor, variant_source);
}

} // namespace framework
