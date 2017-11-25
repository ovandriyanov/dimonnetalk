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
    error_code ec;

    boost::asio::signal_set sigset_term{io_service};
    if(sigset_term.add(SIGTERM, ec)) throw system_error{ec, "signal"};
    if(sigset_term.add(SIGINT, ec)) throw system_error{ec, "signal"};

    boost::asio::signal_set sigset_hup{io_service};
    if(sigset_hup.add(SIGHUP, ec)) throw system_error{ec, "signal"};

    auto inventory = fw::make_inventory("config.json");
    inventory.reload();

    util::callback_wrapper_t callback_wrapper;

    using namespace boost::coroutines2;
    std::unique_ptr<coroutine<void>::pull_type> hup_resume{new coroutine<void>::pull_type{
        [&](coroutine<void>::push_type& yield)
    {
        while(true) {
            BOOST_LOG_TRIVIAL(debug) << "Waiting for SIGHUP";
            auto res = util::async_wait(sigset_hup, callback_wrapper, yield, hup_resume);
            if(auto* ec = boost::get<error_code>(&res))
                throw system_error{*ec, "signal"};
            BOOST_LOG_TRIVIAL(info) << "SIGHUP received, reloading";
            inventory.reload();
        }
    }}};

    std::unique_ptr<coroutine<void>::pull_type> term_resume{new coroutine<void>::pull_type{
        [&](coroutine<void>::push_type& yield)
    {
        BOOST_LOG_TRIVIAL(debug) << "Waiting for SIGTERM/SIGINT";
        auto res = util::async_wait(sigset_term, callback_wrapper, yield, term_resume);
        if(auto* ec = boost::get<error_code>(&res))
            throw system_error{*ec, "signal"};
        BOOST_LOG_TRIVIAL(info) << "Termination signal received, stopping...";
        inventory.stop(io_service, []() { BOOST_LOG_TRIVIAL(info) << "Stopped"; });

        sigset_hup.cancel();
        hup_resume = nullptr;
        callback_wrapper.cancel_callbacks();
    }}};

    io_service.run();

    BOOST_LOG_TRIVIAL(info) << "Exiting with status " << EXIT_SUCCESS;
    return EXIT_SUCCESS;
} catch(const std::exception& ex) {
    BOOST_LOG_TRIVIAL(error) << ex.what();
    BOOST_LOG_TRIVIAL(info) << "Exiting with status " << EXIT_FAILURE;
    return EXIT_FAILURE;
}
