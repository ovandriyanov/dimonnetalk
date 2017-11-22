/*
 *  main.cpp
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 19/11/2017.
 *
 */

#include <boost/asio/signal_set.hpp>
#include <boost/log/trivial.hpp>

#include "framework/inventory.h"

#include <type_traits>

using namespace boost::system;
namespace fw = framework;

int main() try
{
    boost::asio::io_service io_service{1};
    bool stop = false;

    boost::system::error_code ec;

    boost::asio::signal_set sigset_term{io_service};
    sigset_term.add(SIGTERM, ec);
    if(ec) throw system_error{ec, "signal"};
    sigset_term.add(SIGINT, ec);
    if(ec) throw system_error{ec, "signal"};

    boost::asio::signal_set sigset_hup{io_service};
    sigset_term.add(SIGHUP, ec);
    if(ec) throw system_error{ec, "signal"};

    auto inventory = fw::make_inventory();
    inventory.reload();

    sigset_term.async_wait([&](error_code ec, int /* signal */) {
        if(ec) {
            if(ec == boost::asio::error::operation_aborted) return;
            throw system_error{ec, "signal"};
        }
        BOOST_LOG_TRIVIAL(info) << "Termination signal received, stopping...";
        inventory.stop(io_service, []() { BOOST_LOG_TRIVIAL(info) << "Stopped"; });
        sigset_hup.cancel();
    });

    sigset_hup.async_wait([&](error_code ec, int /* signal */) {
        if(ec) {
            if(ec == boost::asio::error::operation_aborted) return;
            throw system_error{ec, "signal"};
        }
        BOOST_LOG_TRIVIAL(info) << "SIGHUP received, reloading";
        inventory.reload();
    });

    io_service.run();

    BOOST_LOG_TRIVIAL(info) << "Exiting with status " << EXIT_SUCCESS;
    return EXIT_SUCCESS;
} catch(const std::exception& ex) {
    BOOST_LOG_TRIVIAL(error) << ex.what();
    BOOST_LOG_TRIVIAL(info) << "Exiting with status " << EXIT_FAILURE;
    return EXIT_FAILURE;
}
