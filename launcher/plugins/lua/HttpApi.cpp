// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

// Lua raises errors (and yields) with longjmp, which skips C++ destructors. The C functions here only hold plain
// pointers across Lua calls that may raise errors: the request being built lives in a static buffer (plugins only run
// on the main thread), and C++ objects are only created in statements that make no such call.

#include "HttpApi.h"

#include <cstring>

#include <lua.hpp>

#include "plugins/Logging.h"
#include "plugins/lua/LuaState.h"
#include "plugins/lua/PrismApi.h"

namespace {

// Registry key of the table that maps each running task (a coroutine) to the reference that keeps it alive
constexpr const char* s_tasksKey = "prism.tasks";
constexpr const char* s_pluginIdKey = "prism.id";
constexpr const char* s_pendingKey = "prism.httpPending";

PrismApi::HttpRequest s_request;

// Logs the error at the top of the dead task co, with its traceback
void reportTaskError(lua_State* L, lua_State* co)
{
    const char* message = lua_type(co, -1) == LUA_TSTRING ? lua_tostring(co, -1) : "(error object is not a string)";
    luaL_traceback(L, co, message, 0);
    lua_getfield(L, LUA_REGISTRYINDEX, s_pluginIdKey);
    qCWarning(pluginLogC).noquote()
        << QString("[%1] async task failed: %2").arg(QString::fromUtf8(lua_tostring(L, -1)), QString::fromUtf8(lua_tostring(L, -2)));
    lua_pop(L, 2);
}

// Resumes the task co, with nargs values already pushed on its stack, and forgets it once it ends
void resumeTask(lua_State* L, lua_State* co, int nargs)
{
    int nresults = 0;
    const int status = lua_resume(co, L, nargs, &nresults);
    if (status == LUA_YIELD) {
        lua_pop(co, nresults);
        return;
    }
    if (status != LUA_OK) {
        reportTaskError(L, co);
    }

    lua_getfield(L, LUA_REGISTRYINDEX, s_tasksKey);
    lua_pushthread(co);
    lua_xmove(co, L, 1);
    lua_pushvalue(L, -1);
    const int ref = lua_rawget(L, -3) == LUA_TNUMBER ? static_cast<int>(lua_tointeger(L, -1)) : LUA_NOREF;
    lua_pop(L, 1);
    lua_pushnil(L);
    lua_rawset(L, -3);
    lua_pop(L, 1);
    luaL_unref(L, LUA_REGISTRYINDEX, ref);
}

bool isTask(lua_State* L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, s_tasksKey);
    lua_pushthread(L);
    const bool task = lua_rawget(L, -2) != LUA_TNIL;
    lua_pop(L, 2);
    return task;
}

// prism.async(fn, ...)
int async(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TFUNCTION);
    const int nargs = lua_gettop(L);
    lua_State* co = lua_newthread(L);
    if (!lua_checkstack(co, nargs)) {
        return luaL_error(L, "too many arguments");
    }

    // Kept alive by a reference while it runs or waits
    lua_getfield(L, LUA_REGISTRYINDEX, s_tasksKey);
    lua_pushvalue(L, -2);
    lua_pushvalue(L, -1);
    const int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_pushinteger(L, ref);
    lua_rawset(L, -3);
    lua_pop(L, 2);  // tasks table, thread

    lua_xmove(L, co, nargs);
    resumeTask(L, co, nargs - 1);
    return 0;
}

// Builds s_request from the arguments, raising errors for invalid ones
void buildRequest(lua_State* L, const char* method, bool hasBody)
{
    const char* url = luaL_checkstring(L, 1);
    size_t bodyLength = 0;
    const char* body = hasBody ? luaL_checklstring(L, 2, &bodyLength) : "";
    const int opts = hasBody ? 3 : 2;
    if (!lua_isnoneornil(L, opts)) {
        luaL_checktype(L, opts, LUA_TTABLE);
    }

    s_request = PrismApi::HttpRequest();
    s_request.method = method;
    s_request.url = QUrl(QString::fromUtf8(url), QUrl::StrictMode);
    s_request.body = QByteArray(body, static_cast<qsizetype>(bodyLength));
    if (!s_request.url.isValid() || (s_request.url.scheme() != "http" && s_request.url.scheme() != "https") ||
        s_request.url.host().isEmpty()) {
        luaL_error(L, "'%s' is not an http or https URL", url);
    }

    if (!lua_istable(L, opts)) {
        return;
    }
    if (lua_getfield(L, opts, "headers") == LUA_TTABLE) {
        const int headers = lua_gettop(L);
        lua_pushnil(L);
        while (lua_next(L, headers) != 0) {
            if (lua_type(L, -2) != LUA_TSTRING || lua_type(L, -1) != LUA_TSTRING) {
                luaL_error(L, "headers must map names to strings");
            }
            size_t nameLength = 0;
            size_t valueLength = 0;
            const char* name = lua_tolstring(L, -2, &nameLength);
            const char* value = lua_tolstring(L, -1, &valueLength);
            // No CR or LF, so a header cannot inject others
            if (nameLength == 0 || std::strlen(name) != nameLength || std::strlen(value) != valueLength || std::strpbrk(name, "\r\n") ||
                std::strpbrk(value, "\r\n")) {
                luaL_error(L, "invalid header '%s'", name);
            }
            s_request.headers.append(
                { QByteArray(name, static_cast<qsizetype>(nameLength)), QByteArray(value, static_cast<qsizetype>(valueLength)) });
            lua_pop(L, 1);
        }
    } else if (!lua_isnil(L, -1)) {
        luaL_error(L, "'headers' must be a table");
    }
    lua_pop(L, 1);

    if (lua_getfield(L, opts, "timeout") != LUA_TNIL) {
        const lua_Number timeout = lua_tonumber(L, -1);
        if (!lua_isnumber(L, -1) || timeout <= 0 || timeout > PrismApi::s_maxHttpTimeoutSeconds) {
            luaL_error(L, "'timeout' must be between 0 and %d seconds", PrismApi::s_maxHttpTimeoutSeconds);
        }
        s_request.timeoutMs = static_cast<int>(timeout * 1000);
    }
    lua_pop(L, 1);
}

struct CompleteArgs {
    int ref;
    const PrismApi::HttpResponse* response;
    // response->error, already converted so no C++ object is created while pushing it
    QByteArray error;
};

void pushResponse(lua_State* L, const PrismApi::HttpResponse& response, const QByteArray& error)
{
    lua_createtable(L, 0, 5);
    lua_pushinteger(L, response.status);
    lua_setfield(L, -2, "status");
    lua_pushboolean(L, response.error.isEmpty() && response.status >= 200 && response.status < 300);
    lua_setfield(L, -2, "ok");
    lua_pushlstring(L, response.body.constData(), response.body.size());
    lua_setfield(L, -2, "body");
    lua_createtable(L, 0, static_cast<int>(response.headers.size()));
    for (const auto& [name, value] : response.headers) {
        lua_pushlstring(L, value.constData(), value.size());
        lua_setfield(L, -2, name.constData());
    }
    lua_setfield(L, -2, "headers");
    if (!error.isEmpty()) {
        lua_pushlstring(L, error.constData(), error.size());
        lua_setfield(L, -2, "error");
    }
}

// Resumes the task waiting for a request. Argument 1 is a light userdata pointing to the CompleteArgs.
int completeRequest(lua_State* L)
{
    const auto* args = static_cast<const CompleteArgs*>(lua_touserdata(L, 1));
    lua_rawgeti(L, LUA_REGISTRYINDEX, args->ref);
    lua_State* co = lua_tothread(L, -1);
    luaL_unref(L, LUA_REGISTRYINDEX, args->ref);

    lua_getfield(L, LUA_REGISTRYINDEX, s_pendingKey);
    lua_pushinteger(L, lua_tointeger(L, -1) - 1);
    lua_setfield(L, LUA_REGISTRYINDEX, s_pendingKey);
    lua_pop(L, 1);

    if (!co || lua_status(co) != LUA_YIELD) {
        return 0;
    }
    pushResponse(L, *args->response, args->error);
    if (!lua_checkstack(co, 1)) {
        return luaL_error(L, "stack overflow");
    }
    lua_xmove(L, co, 1);
    resumeTask(L, co, 1);
    return 0;
}

// prism.http.get/post: upvalue 1 tells whether the plugin has the net permission, upvalue 2 is a light userdata
// pointing to the HttpClient (or null)
int request(lua_State* L, const char* method, bool hasBody)
{
    if (!lua_toboolean(L, lua_upvalueindex(1))) {
        return luaL_error(L, "prism.http requires the 'net' permission");
    }
    if (!lua_isyieldable(L) || !isTask(L)) {
        return luaL_error(L, "prism.http must be called from a task started with prism.async");
    }
    auto* client = static_cast<PrismApi::HttpClient*>(lua_touserdata(L, lua_upvalueindex(2)));
    if (!client) {
        return luaL_error(L, "the network is not available");
    }
    lua_getfield(L, LUA_REGISTRYINDEX, s_pendingKey);
    const lua_Integer pending = lua_tointeger(L, -1);
    lua_pop(L, 1);
    if (pending >= PrismApi::s_maxPendingHttpRequests) {
        return luaL_error(L, "too many requests at once, the maximum is %d", PrismApi::s_maxPendingHttpRequests);
    }

    buildRequest(L, method, hasBody);

    // This task is resumed through this reference when the response arrives
    lua_pushthread(L);
    const int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_pushinteger(L, pending + 1);
    lua_setfield(L, LUA_REGISTRYINDEX, s_pendingKey);

    LuaState* state = LuaState::fromLua(L);
    client->send(s_request, [state, lifetime = state->lifetime(), ref](const PrismApi::HttpResponse& response) {
        // The plugin may have been unloaded meanwhile
        if (lifetime.expired()) {
            return;
        }
        CompleteArgs args{ ref, &response, response.error.toUtf8() };
        lua_State* main = state->get();
        lua_pushcfunction(main, completeRequest);
        lua_pushlightuserdata(main, &args);
        if (auto error = state->call(1)) {
            qCWarning(pluginLogC).noquote() << "Could not resume a plugin task:" << *error;
        }
    });
    return lua_yield(L, 0);
}

int httpGet(lua_State* L)
{
    return request(L, "GET", false);
}

int httpPost(lua_State* L)
{
    return request(L, "POST", true);
}

}  // namespace

namespace PrismApi {

void setUpTasks(lua_State* L, const QByteArray& pluginId)
{
    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, s_tasksKey);
    lua_pushlstring(L, pluginId.constData(), pluginId.size());
    lua_setfield(L, LUA_REGISTRYINDEX, s_pluginIdKey);
    lua_pushinteger(L, 0);
    lua_setfield(L, LUA_REGISTRYINDEX, s_pendingKey);
}

void pushAsyncFunction(lua_State* L)
{
    lua_pushcfunction(L, async);
}

void pushHttpTable(lua_State* L, bool allowed, HttpClient* client)
{
    lua_createtable(L, 0, 2);
    lua_pushboolean(L, allowed);
    lua_pushlightuserdata(L, client);
    lua_pushcclosure(L, httpGet, 2);
    lua_setfield(L, -2, "get");
    lua_pushboolean(L, allowed);
    lua_pushlightuserdata(L, client);
    lua_pushcclosure(L, httpPost, 2);
    lua_setfield(L, -2, "post");
}

}  // namespace PrismApi
