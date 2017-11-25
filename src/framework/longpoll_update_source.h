/*
 *  framework/longpoll_update_source.h
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 24/11/2017.
 *
 */

#ifndef FRAMEWORK_LONGPOLL_UPDATE_SOURCE_H
#define FRAMEWORK_LONGPOLL_UPDATE_SOURCE_H

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <nlohmann/json.hpp>

#include "framework/config.h"
#include "util/asio.h"

namespace framework {

class longpoll_update_source_t
{
public:
     class delegate_t {
     public:
         virtual void new_update(nlohmann::json update) = 0;
     };

public:
    longpoll_update_source_t(boost::asio::io_service& io_service,
                             const longpoll_config_t& config,
                             delegate_t& delegate);

    void reload();
    void stop();

private:
    const longpoll_config_t& config_;
    delegate_t& delegate_;

    boost::asio::ip::tcp::resolver resolver_;
    boost::asio::ssl::context ssl_context_;
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_stream_;
    std::string host_;
};

} // namespace framework

#endif // FRAMEWORK_LONGPOLL_UPDATE_SOURCE_H
