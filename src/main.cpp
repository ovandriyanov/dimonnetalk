/*
 *  main.cpp
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 19/11/2017.
 *
 */

#include <boost/asio/signal_set.hpp>
#include <boost/coroutine2/all.hpp>
#include <boost/log/trivial.hpp>

#include "framework/inventory.h"
#include "util/asio.h"

#include <type_traits>

using namespace boost::system;
namespace fw = framework;

int main() try
{
    boost::asio::io_service io_service{1};

    util::signal_set_t sigset_term{io_service, SIGTERM, SIGINT};
    util::signal_set_t sigset_hup{io_service, SIGHUP};

    auto inventory = fw::make_inventory();
    inventory.reload();

    util::callback_wrapper_t callback_wrapper;

    bool stop = false;
    util::coro_t<void>::pull_type hup_resume{[&](util::coro_t<void>::push_type& yield) {
        while(true) {
            BOOST_LOG_TRIVIAL(debug) << "Waiting for SIGHUP";
            sigset_hup.async_wait(yield, hup_resume);
            if(stop) return;

            BOOST_LOG_TRIVIAL(info) << "SIGHUP received, reloading";
            inventory.reload();
        }
    }};

    util::coro_t<void>::pull_type term_resume([&](util::coro_t<void>::push_type& yield) {
        BOOST_LOG_TRIVIAL(debug) << "Waiting for SIGTERM";
        int sig = sigset_term.async_wait(yield, term_resume);
        BOOST_LOG_TRIVIAL(info) << "Termination signal received, stopping...";
        inventory.stop(io_service, []() { BOOST_LOG_TRIVIAL(info) << "Stopped"; });

        stop = true;
        sigset_hup.cancel(hup_resume);
    });

    io_service.run();

    BOOST_LOG_TRIVIAL(info) << "Exiting with status " << EXIT_SUCCESS;
    return EXIT_SUCCESS;
} catch(const std::exception& ex) {
    BOOST_LOG_TRIVIAL(error) << ex.what();
    BOOST_LOG_TRIVIAL(info) << "Exiting with status " << EXIT_FAILURE;
    return EXIT_FAILURE;
}
