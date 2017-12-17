/*
 *  framework/lua/framework.cpp
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 17/12/2017.
 *
 */

#include "framework/lua/framework.h"
#include "framework/lua/globals.h"
#include "framework/lua/util.h"

namespace framework {
namespace lua {

static int get_update(lua_State* lua_state)
{
    if(lua_gettop(lua_state) != 0)
        return luaL_error(lua_state, "get_update takes no arguments");

    auto& globals = get_from_registry<globals_t>(lua_state, globals_name);
    globals.bot->update_source().get_update(globals.bot->wrap([=](std::exception_ptr ep, nlohmann::json json) {
        // TODO: handle error
        auto ret = json.dump();
        lua_pushlstring(lua_state, ret.c_str(), ret.size());
        resume(lua_state, 1);
    }));

    return lua_yield(lua_state, 0);
}

static int call_api(lua_State* lua_state)
{
    try {
        const char* call_cstr = luaL_checkstring(lua_state, -2);
        const char* request_cstr = luaL_checkstring(lua_state, -1);

        auto& globals = get_from_registry<globals_t>(lua_state, globals_name);
        api_server_t::request_t req;
        req.api_token = globals.bot->bot_config().api_token;
        req.call = call_cstr;
        req.request = nlohmann::json::parse(request_cstr);

        req.callback = [=](std::exception_ptr ep, nlohmann::json json) {
            if(ep) {
                lua_pushboolean(lua_state, false);
                try {
                    std::rethrow_exception(ep);
                } catch(const std::exception& ex) {
                    lua_pushstring(lua_state, ex.what());
                }
            } else {
                lua_pushboolean(lua_state, true);
                lua_pushstring(lua_state, json.dump().c_str());
            }
            resume(lua_state, 2);
        };

        globals.bot->api_server().call_api(req);
        return lua_yieldk(lua_state, 0, 0, [](lua_State* lua_state, int status, lua_KContext ctx) {
            bool ok = lua_toboolean(lua_state, -2);
            if(!ok)
                return luaL_error(lua_state, "Cannot call API: %s", lua_tostring(lua_state, -1));
            lua_remove(lua_state, -2);
            return 1;
        });
    } catch(const nlohmann::json::exception& ex) {
        return luaL_error(lua_state, "Cannot parse JSON request string: %s", ex.what());
    }
}

int push_framework_table(lua_State* lua_state)
{
    luaL_Reg framework_functions[] = {
        {"get_update", get_update},
        {"call_api", call_api},
        {nullptr, nullptr}
    };

    lua_newtable(lua_state);
    luaL_newlib(lua_state, framework_functions);

    return 1;
}

} // namespace lua
} // namespace framework
