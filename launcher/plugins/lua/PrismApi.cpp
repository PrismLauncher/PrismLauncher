// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

// Lua raises errors with longjmp, which skips C++ destructors. The C functions here never keep a C++ object alive
// across a Lua call that may raise an error (anything that allocates, calls metamethods or is luaL_check*/luaL_error).

#include "PrismApi.h"

#include <QDir>

#include <algorithm>
#include <array>
#include <cstring>

#include <lua.hpp>

#include "plugins/Logging.h"
#include "plugins/PluginManifest.h"
#include "plugins/lua/LuaState.h"
#include "plugins/lua/SettingsApi.h"

namespace {

enum class LogLevel { Info, Warning, Error };

// Builds the message from the strings at stack indices [first, last]. Makes no Lua call that raises errors.
QString joinLogMessage(lua_State* L, int first, int last)
{
    QString message;
    for (int i = first; i <= last; i++) {
        size_t length = 0;
        const char* text = lua_tolstring(L, i, &length);
        if (i > first) {
            message += ' ';
        }
        // Only convert what fits, the string may be as large as the plugin's memory limit
        const qsizetype remaining = std::max<qsizetype>(0, PrismApi::s_maxLogMessageLength - message.size());
        const bool truncated = static_cast<qsizetype>(length) > remaining;
        message += QString::fromUtf8(text, truncated ? remaining : static_cast<qsizetype>(length));
        if (truncated) {
            message += " [truncated]";
            break;
        }
    }
    return message;
}

// prism.log.<level>(...): upvalue 1 is the plugin id, upvalue 2 the level
int log(lua_State* L)
{
    const int nargs = lua_gettop(L);
    luaL_checkstack(L, nargs, "too many arguments");
    // Same conversion as print, honoring __tostring. Done before building any C++ object, as it may raise errors.
    for (int i = 1; i <= nargs; i++) {
        luaL_tolstring(L, i, nullptr);
    }

    const QString line =
        QString("[%1] %2").arg(QString::fromUtf8(lua_tostring(L, lua_upvalueindex(1))), joinLogMessage(L, nargs + 1, nargs * 2));
    switch (static_cast<LogLevel>(lua_tointeger(L, lua_upvalueindex(2)))) {
        case LogLevel::Info:
            qCInfo(pluginLogC).noquote() << line;
            break;
        case LogLevel::Warning:
            qCWarning(pluginLogC).noquote() << line;
            break;
        case LogLevel::Error:
            qCCritical(pluginLogC).noquote() << line;
            break;
    }
    return 0;
}

// Pushes the prism.log.<level> function for the plugin id at the top of the stack
void pushLogFunction(lua_State* L, LogLevel level)
{
    lua_pushvalue(L, -1);
    lua_pushinteger(L, static_cast<lua_Integer>(level));
    lua_pushcclosure(L, log, 2);
}

// Registry key of the table that maps event names to arrays of handlers
constexpr const char* s_handlersKey = "prism.handlers";
constexpr std::array s_events = { "launcher.ready" };

bool isKnownEvent(const char* event)
{
    return std::any_of(s_events.begin(), s_events.end(), [event](const char* known) { return std::strcmp(event, known) == 0; });
}

// prism.on(event, handler)
int on(lua_State* L)
{
    const char* event = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    if (!isKnownEvent(event)) {
        return luaL_error(L, "unknown event '%s'", event);
    }

    lua_getfield(L, LUA_REGISTRYINDEX, s_handlersKey);
    if (lua_getfield(L, -1, event) == LUA_TNIL) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, -3, event);
    }
    lua_pushvalue(L, 2);
    lua_rawseti(L, -2, static_cast<lua_Integer>(lua_rawlen(L, -2)) + 1);
    return 0;
}

// Calls the handlers of an event. Argument 1 is a light userdata pointing to the event name.
int dispatch(lua_State* L)
{
    const char* event = static_cast<const char*>(lua_touserdata(L, 1));
    lua_getfield(L, LUA_REGISTRYINDEX, s_handlersKey);
    if (lua_getfield(L, -1, event) != LUA_TTABLE) {
        return 0;
    }
    // Handlers registered meanwhile only run from the next event on
    const auto count = static_cast<lua_Integer>(lua_rawlen(L, -1));
    for (lua_Integer i = 1; i <= count; i++) {
        lua_rawgeti(L, -1, i);
        lua_call(L, 0, 0);
    }
    return 0;
}

struct InstallArgs {
    QByteArray id;
    QByteArray settingsPath;
};

// Argument 1 is a light userdata pointing to the InstallArgs
int openPrism(lua_State* L)
{
    const auto* args = static_cast<const InstallArgs*>(lua_touserdata(L, 1));
    lua_newtable(L);  // prism

    lua_newtable(L);  // prism.log
    lua_pushlstring(L, args->id.constData(), args->id.size());
    pushLogFunction(L, LogLevel::Info);
    lua_setfield(L, -3, "info");
    pushLogFunction(L, LogLevel::Warning);
    lua_setfield(L, -3, "warn");
    pushLogFunction(L, LogLevel::Error);
    lua_setfield(L, -3, "error");

    pushLogFunction(L, LogLevel::Info);
    lua_setglobal(L, "print");
    lua_pop(L, 1);  // plugin id
    lua_setfield(L, -2, "log");

    PrismApi::pushSettingsFunction(L, args->settingsPath);
    lua_setfield(L, -2, "settings");

    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, s_handlersKey);
    lua_pushcfunction(L, on);
    lua_setfield(L, -2, "on");

    lua_setglobal(L, "prism");
    return 0;
}

}  // namespace

namespace PrismApi {

std::optional<QString> install(LuaState& state, const PluginManifest& manifest, const QString& directory)
{
    // Everything that allocates Lua memory happens inside the protected call, so running out of memory cannot panic
    const InstallArgs args{ manifest.id.toUtf8(), QDir(directory).filePath("settings.ini").toUtf8() };
    lua_State* L = state.get();
    lua_pushcfunction(L, openPrism);
    lua_pushlightuserdata(L, const_cast<InstallArgs*>(&args));
    if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
        const QString error = QString::fromUtf8(lua_tostring(L, -1));
        lua_pop(L, 1);
        return error;
    }
    return std::nullopt;
}

std::optional<QString> emitEvent(LuaState& state, const char* event)
{
    lua_State* L = state.get();
    lua_pushcfunction(L, dispatch);
    lua_pushlightuserdata(L, const_cast<char*>(event));
    return state.call(1);
}

}  // namespace PrismApi
