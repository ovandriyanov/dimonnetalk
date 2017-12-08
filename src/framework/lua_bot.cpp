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

using namespace boost::system;

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

    luaL_requiref(lua_state_.get(), "dimonnetalk.log", setup_log, false);
    lua_pop(lua_state_.get(), 1);

    luaL_requiref(lua_state_.get(), "dimonnetalk.io", setup_io, false);
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
    resume_ = nullptr;
    lua_state_ = nullptr;
}

int lua_bot_t::setup_log(lua_State* lua_state)
{
    lua_newtable(lua_state);
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
    return 1;
}

template <typename T>
static T& to_userdata(lua_State* lua_state, int pos)
{
    assert(lua_islightuserdata(lua_state, pos) || lua_isuserdata(lua_state, pos));
    auto& ret = *static_cast<T*>(lua_touserdata(lua_state, pos));
    return ret;
}

template <typename T>
static int destroy_userdata(lua_State* lua_state)
{
    auto& object = to_userdata<T>(lua_state, -1);
    lua_pop(lua_state, 1);
    object.~T();
    return 0;
}

template <typename T>
static T& get_from_registry(lua_State* lua_state, const char* name)
{
    lua_getfield(lua_state, LUA_REGISTRYINDEX, name);
    auto& ret = to_userdata<T>(lua_state, -1);
    lua_pop(lua_state, 1);
    return ret;
}

template <typename T, typename... Args>
static void push_userdata(lua_State* lua_state, const char* metatable_name, Args&&... args)
{
    void* ptr = lua_newuserdata(lua_state, sizeof(T));
    auto* timer = new(ptr) T{std::forward<Args>(args)...};
    luaL_setmetatable(lua_state, metatable_name);
}

int lua_bot_t::setup_io(lua_State* lua_state)
{
    class timer_t : public util::callback_wrapper_t
    {
    public:
        timer_t(boost::asio::io_service& io_service) : timer{io_service} {}
        static int destroy(lua_State* lua_state)
        {
            auto& self = to_userdata<timer_t>(lua_state, -1);
            for(int ref : self.callback_refs)
                luaL_unref(lua_state, LUA_REGISTRYINDEX, ref);
            self.~timer_t();
            lua_pop(lua_state, 1);
            return 0;
        }

    public:
        boost::asio::steady_timer timer;
        std::list<int> callback_refs;
    };

    auto timer_expires_from_now = [](lua_State* lua_state) {
        luaL_checkudata(lua_state, -2, "dimonnetalk.io.timer");
        auto& timer = to_userdata<timer_t>(lua_state, -2);
        auto ms = luaL_checkinteger(lua_state, -1);
        lua_pop(lua_state, 2);

        timer.timer.expires_after(std::chrono::milliseconds{ms});
        return 0;
    };

    auto timer_async_wait = [](lua_State* lua_state) {
        timer_t* timer;
        if(lua_gettop(lua_state) == 2) { // callback overload
            luaL_checkudata(lua_state, -2, "dimonnetalk.io.timer");
            timer = &to_userdata<timer_t>(lua_state, -2);
            luaL_argcheck(lua_state, lua_isfunction(lua_state, -1), -1, "argument must be a function");

            timer->callback_refs.emplace_back(luaL_ref(lua_state, LUA_REGISTRYINDEX));
            timer->timer.async_wait(timer->wrap(
                [=, cbit = std::prev(timer->callback_refs.end())](const error_code& ec)
            {
                if(ec) throw system_error{ec, "timer"};

                int cb_ref = *cbit;
                timer->callback_refs.erase(cbit);

                auto* thread = lua_newthread(lua_state);
                lua_pop(lua_state, 1); // TODO: save reference to this thread
                lua_rawgeti(thread, LUA_REGISTRYINDEX, cb_ref);
                lua_call(thread, 0, 0);
            }));
        } else { // coroutine overload
            luaL_checkudata(lua_state, -1, "dimonnetalk.io.timer");
            timer = &to_userdata<timer_t>(lua_state, -1);
            auto& bot = get_from_registry<lua_bot_t>(lua_state, "bot");
            auto& yield = get_from_registry<boost::coroutines2::coroutine<void>::pull_type>(lua_state, "yield");

            util::async_wait_timer(timer->timer, yield, *bot.resume_);
        }

        return 0;
    };

    auto timer_new = [](lua_State* lua_state) {
        push_userdata<timer_t>(
            lua_state, "dimonnetalk.io.timer", get_from_registry<boost::asio::io_service>(lua_state, "io_service"));
        return 1;
    };

    luaL_Reg timer_methods[] = {
        {"expires_from_now", timer_expires_from_now},
        {"async_wait", timer_async_wait},
        // {"cancel", timer_cancel},
        {"__gc", destroy_userdata<timer_t>},
        {nullptr, nullptr}
    };

    luaL_Reg timer_functions[] = {
        {"new", timer_new},
        {nullptr, nullptr}
    };

    int exists = !luaL_newmetatable(lua_state, "dimonnetalk.io.timer");
    assert(!exists);
    lua_pushvalue(lua_state, -1);
    lua_setfield(lua_state, -2, "__index");
    luaL_setfuncs(lua_state, timer_methods, 0);
    lua_pop(lua_state, 1);

    lua_newtable(lua_state);

    luaL_newlib(lua_state, timer_functions);
    lua_setfield(lua_state, -2, "timer");

    class promise_t
    {
    public:
        promise_t() : resume{nullptr} {}

    public:
        boost::optional<int> value_ref;
        boost::coroutines2::coroutine<void>::push_type* resume;
    };

    auto promise_set_value = [](lua_State* lua_state) {
        luaL_checkudata(lua_state, -2, "dimonnetalk.io.promise");
        auto& promise = to_userdata<promise_t>(lua_state, -2);
        luaL_checkany(lua_state, -1);

        if(promise.value_ref) return 0; // value is already set
        promise.value_ref = luaL_ref(lua_state, LUA_REGISTRYINDEX);
        if(promise.resume) { // waiting for value
            auto* resume = promise.resume;
            promise.resume = nullptr;
            (*resume)();
        }
        return 0;
    };

    auto promise_get_value = [](lua_State* lua_state) {
        luaL_checkudata(lua_state, -1, "dimonnetalk.io.promise");
        auto& promise = to_userdata<promise_t>(lua_state, -1);

        if(!promise.value_ref) {
            // wait for value
            assert(!promise.resume);
            auto& bot = get_from_registry<lua_bot_t>(lua_state, "bot");
            promise.resume = bot.resume_.get();
            auto& yield = get_from_registry<boost::coroutines2::coroutine<void>::push_type>(lua_state, "yield");
            yield();
        }
        assert(promise.value_ref);
        promise.resume = nullptr;
        lua_rawgeti(lua_state, LUA_REGISTRYINDEX, *promise.value_ref);
        return 1;
    };

    auto promise_destroy = [](lua_State* lua_state) {
        luaL_checkudata(lua_state, -1, "dimonnetalk.io.promise");
        auto& promise = to_userdata<promise_t>(lua_state, -1);
        if(promise.value_ref)
            luaL_unref(lua_state, LUA_REGISTRYINDEX, *promise.value_ref);
        promise.~promise_t();
        return 0;
    };

    auto promise_new = [](lua_State* lua_state) {
        push_userdata<promise_t>(lua_state, "dimonnetalk.io.promise");
        return 1;
    };

    luaL_Reg promise_methods[] = {
        {"get_value", promise_get_value},
        {"set_value", promise_set_value},
        {"__gc", promise_destroy},
        {"__gc", promise_destroy},
        {nullptr, nullptr}
    };

    luaL_Reg promise_functions[] = {
        {"new", promise_new},
        {nullptr, nullptr}
    };

    exists = !luaL_newmetatable(lua_state, "dimonnetalk.io.promise");
    assert(!exists);
    lua_pushvalue(lua_state, -1);
    lua_setfield(lua_state, -2, "__index");
    luaL_setfuncs(lua_state, promise_methods, 0);
    lua_pop(lua_state, 1);

    luaL_newlib(lua_state, promise_functions);
    lua_setfield(lua_state, -2, "promise");

    return 1;
}

} // namespace framework
