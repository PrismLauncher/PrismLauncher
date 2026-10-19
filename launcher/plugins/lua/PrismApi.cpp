// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "PrismApi.h"

#include <algorithm>

#include <lua.hpp>

#include "plugins/Logging.h"
#include "plugins/PluginManifest.h"
#include "plugins/lua/LuaState.h"

namespace {

enum class LogLevel { Info, Warning, Error };

// prism.log.<level>(...): upvalue 1 is the plugin id, upvalue 2 the level
int log(lua_State* L)
{
    const int nargs = lua_gettop(L);
    QString message;
    for (int i = 1; i <= nargs; i++) {
        size_t length = 0;
        // Same conversion as print: honors __tostring
        const char* text = luaL_tolstring(L, i, &length);
        if (i > 1) {
            message += ' ';
        }
        // Only convert what fits, the string may be as large as the plugin's memory limit
        const qsizetype remaining = std::max<qsizetype>(0, PrismApi::s_maxLogMessageLength - message.size());
        const bool truncated = static_cast<qsizetype>(length) > remaining;
        message += QString::fromUtf8(text, truncated ? remaining : static_cast<qsizetype>(length));
        lua_pop(L, 1);
        if (truncated) {
            message += " [truncated]";
            break;
        }
    }

    const QString line = QString("[%1] %2").arg(QString::fromUtf8(lua_tostring(L, lua_upvalueindex(1))), message);
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

// Argument 1 is the plugin id
int openPrism(lua_State* L)
{
    luaL_checkstring(L, 1);
    lua_newtable(L);  // prism

    lua_newtable(L);  // prism.log
    lua_pushvalue(L, 1);
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
    lua_setglobal(L, "prism");
    return 0;
}

}  // namespace

namespace PrismApi {

std::optional<QString> install(LuaState& state, const PluginManifest& manifest)
{
    lua_State* L = state.get();
    lua_pushcfunction(L, openPrism);
    const QByteArray id = manifest.id.toUtf8();
    lua_pushlstring(L, id.constData(), id.size());
    if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
        const QString error = QString::fromUtf8(lua_tostring(L, -1));
        lua_pop(L, 1);
        return error;
    }
    return std::nullopt;
}

}  // namespace PrismApi
