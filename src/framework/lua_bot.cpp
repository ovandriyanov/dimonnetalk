/*
 *  framework/lua_bot.cpp
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 03/12/2017.
 *
 */

#include <boost/asio/steady_timer.hpp>
#include <boost/log/trivial.hpp>

#include "framework/lua_bot.h"
#include "util/asio.h"

namespace framework {

lua_bot_t::lua_bot_t(boost::asio::io_service& io_service,
                     const bot_config_t& bot_config,
                     const lua_bot_config_t& lua_bot_config)
    : io_service_{io_service}
    , bot_config_{bot_config}
    , lua_bot_config_{lua_bot_config}
    , lua_state_{nullptr, lua_close}
{
}

static int log(lua_State* lua_state)
{
    #define LOG(LEVEL)                                                                             \
    BOOST_LOG_STREAM_WITH_PARAMS(                                                                  \
        boost::log::trivial::logger::get(),                                                        \
        (boost::log::keywords::severity = static_cast<boost::log::trivial::severity_level>(LEVEL)) \
    )

    if(lua_gettop(lua_state) != 1) {
        lua_pushstring(lua_state, "Log function receives exactly one argument");
        lua_error(lua_state);
    }

    if(lua_type(lua_state, -1) != LUA_TSTRING) {
        lua_pushstring(lua_state, "Argument no a log function is not a string");
        lua_error(lua_state);
    }

    size_t len;
    const char* data = lua_tolstring(lua_state, -1, &len);
    std::string s{data, len};

    LOG(lua_tointeger(lua_state, lua_upvalueindex(1))) << s;

    return 0;

    #undef LOG
}



// bot_t
void lua_bot_t::reload()
{
    if(resume_) {
        if(script_path_ == lua_bot_config_.script_path)
            return;
    }

    script_path_ = lua_bot_config_.script_path;

    resume_ = nullptr;
    lua_state_.reset(luaL_newstate());
    if(!lua_state_) throw std::runtime_error{"Cannot create Lua state"};

    luaL_openlibs(lua_state_.get());
    if(int ec = luaL_loadfile(lua_state_.get(), script_path_.c_str())) {
        throw std::runtime_error{"Cannot load Lua script file " + script_path_.string() +
                                 ": " + lua_tostring(lua_state_.get(), -1)};
    }

    lua_pushlightuserdata(lua_state_.get(), &io_service_);
    lua_setfield(lua_state_.get(), LUA_REGISTRYINDEX, "io_service");

    auto open_log = [](lua_State* lua_state) {
        lua_newtable(lua_state);
        setup_log(lua_state);
        return 1;
    };
    luaL_requiref(lua_state_.get(), "dimonnetalk.log", open_log, false);
    lua_pop(lua_state_.get(), 1);

    auto open_io = [](lua_State* lua_state) {
        return setup_io(lua_state);
    };
    luaL_requiref(lua_state_.get(), "dimonnetalk.io", open_io, false);
    lua_pop(lua_state_.get(), 1);

    resume_ = std::make_unique<util::push_coro_t>([this](boost::coroutines2::coroutine<void>::pull_type& yield) {
        lua_pushlightuserdata(lua_state_.get(), &yield);
        lua_setfield(lua_state_.get(), LUA_REGISTRYINDEX, "yield");

        lua_pushlightuserdata(lua_state_.get(), this);
        lua_setfield(lua_state_.get(), LUA_REGISTRYINDEX, "bot");

        if(lua_pcall(lua_state_.get(), 0, 0, 0)) {
            throw std::runtime_error{"Cannot execute Lua script file " + script_path_.string() +
                                     ": " + lua_tostring(lua_state_.get(), -1)};
        }
    });

    (*resume_)();
}

void lua_bot_t::stop()
{

}

void lua_bot_t::setup_log(lua_State* lua_state)
{
    std::tuple<boost::log::trivial::severity_level, const char*> levels[] = {
        {boost::log::trivial::trace,   "trace"},
        {boost::log::trivial::debug,   "debug"},
        {boost::log::trivial::info,    "info"},
        {boost::log::trivial::warning, "warning"},
        {boost::log::trivial::error,   "error"},
        {boost::log::trivial::fatal,   "fatal"}
    };

    for(auto level : levels) {
        lua_CFunction x;
        lua_pushinteger(lua_state, std::get<0>(level));
        lua_pushcclosure(lua_state, log, 1);
        lua_setfield(lua_state, -2, std::get<1>(level));
    }
}

template <typename T>
static T& pop_userdata(lua_State* lua_state)
{
    assert(lua_islightuserdata(lua_state, -1) || lua_isuserdata(lua_state, -1));
    auto& ret = *static_cast<T*>(lua_touserdata(lua_state, -1));
    lua_pop(lua_state, 1);
    return ret;
}

template <typename T>
static int destroy_userdata(lua_State* lua_state)
{
    auto& object = pop_userdata<T>(lua_state);
    object.~T();
    return 0;
}

template <typename T>
static T& get_from_registry(lua_State* lua_state, const char* name)
{
    lua_getfield(lua_state, LUA_REGISTRYINDEX, name);
    return pop_userdata<T>(lua_state);
}

int lua_bot_t::setup_io(lua_State* lua_state)
{
    auto timer_foo = [](lua_State* lua_state) {
        BOOST_LOG_TRIVIAL(debug) << "foo called";
        return 0;
    };

    auto timer_async_wait = [](lua_State* lua_state) {
        auto ms = luaL_checkinteger(lua_state, -1);
        lua_pop(lua_state, 1);
        auto& timer = pop_userdata<boost::asio::steady_timer>(lua_state);
        auto& bot = get_from_registry<lua_bot_t>(lua_state, "bot");
        auto& yield = get_from_registry<boost::coroutines2::coroutine<void>::pull_type>(lua_state, "yield");
        timer.expires_from_now(std::chrono::milliseconds{ms});
        util::async_wait_timer(timer, yield, *bot.resume_);
        return 0;
    };

    luaL_Reg timer_methods[] = {
        {"foo", timer_foo},
        {"async_wait", timer_async_wait},
        {"__gc", destroy_userdata<boost::asio::steady_timer>},
        {nullptr, nullptr}
    };

    int exists = !luaL_newmetatable(lua_state, "dimonnetalk.io.timer");
    assert(!exists);
    lua_pushvalue(lua_state, -1);
    lua_setfield(lua_state, -2, "__index");
    luaL_setfuncs(lua_state, timer_methods, 0);
    lua_pop(lua_state, 1);

    auto timer_new = [](lua_State* lua_state) {
        void* ptr = lua_newuserdata(lua_state, sizeof(boost::asio::steady_timer));
        auto* timer = new(ptr) boost::asio::steady_timer{get_from_registry<boost::asio::io_service>(lua_state, "io_service")};
        luaL_setmetatable(lua_state, "dimonnetalk.io.timer");
        return 1;
    };

    luaL_Reg timer_functions[] = {
        {"new", timer_new},
        {nullptr, nullptr}
    };

    lua_newtable(lua_state);
    luaL_newlib(lua_state, timer_functions);
    lua_setfield(lua_state, -2, "timer");
    return 1;
}

} // namespace framework
