/*
 *  framework/longpoll_update_source.cpp
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 24/11/2017.
 *
 */

#include "framework/longpoll_update_source.h"

namespace framework {

longpoll_update_source_t::longpoll_update_source_t(boost::asio::io_service& io_service,
                                                   const longpoll_config_t& config,
                                                   delegate_t& delegate)
    : config_{config}
    , delegate_{delegate}
    , resolver_{io_service}
    , ssl_context_{boost::asio::ssl::context::sslv23}
    , ssl_stream_{io_service, ssl_context_}
{
}

void longpoll_update_source_t::reload()
{
    assert(config_.host.size());
    if(config_.host == host_) return;

    ssl_stream_.lowest_layer().async_connect
}

void longpoll_update_source_t::stop()
{

}

} // namespace framework
