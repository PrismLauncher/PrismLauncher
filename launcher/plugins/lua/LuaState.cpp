// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "LuaState.h"

#include <QDebug>

#include <algorithm>

#include <lua.hpp>

namespace {

// Wraps the original `load` (upvalue 1) forcing mode "t", so bytecode can never be loaded.
int textOnlyLoad(lua_State* L)
{
    // load(chunk [, chunkname [, mode [, env]]]): keep env absent if it was absent, as `load` treats nil differently
    const int nargs = std::max(lua_gettop(L), 3);
    lua_settop(L, nargs);
    lua_pushliteral(L, "t");
    lua_replace(L, 3);
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_insert(L, 1);
    lua_call(L, nargs, LUA_MULTRET);
    return lua_gettop(L);
}

int openSafeLibs(lua_State* L)
{
    luaL_openselectedlibs(L, LUA_GLIBK | LUA_COLIBK | LUA_MATHLIBK | LUA_STRLIBK | LUA_TABLIBK | LUA_UTF8LIBK, 0);

    lua_pushnil(L);
    lua_setglobal(L, "dofile");
    lua_pushnil(L);
    lua_setglobal(L, "loadfile");

    lua_getglobal(L, "load");
    lua_pushcclosure(L, textOnlyLoad, 1);
    lua_setglobal(L, "load");
    return 0;
}

// Same as the standalone interpreter: stringify the error object and append a traceback.
int messageHandler(lua_State* L)
{
    const char* msg = lua_tostring(L, 1);
    if (!msg) {
        if (luaL_callmeta(L, 1, "__tostring") && lua_type(L, -1) == LUA_TSTRING) {
            return 1;
        }
        msg = lua_pushfstring(L, "(error object is a %s value)", luaL_typename(L, 1));
    }
    luaL_traceback(L, L, msg, 1);
    return 1;
}

}  // namespace

LuaState::LuaState() : m_state(luaL_newstate())
{
    if (!m_state) {
        qWarning() << "Failed to create Lua state";
        return;
    }

    lua_pushcfunction(m_state, openSafeLibs);
    if (lua_pcall(m_state, 0, 0, 0) != LUA_OK) {
        qWarning() << "Failed to open Lua libraries:" << lua_tostring(m_state, -1);
        lua_close(m_state);
        m_state = nullptr;
    }
}

LuaState::~LuaState()
{
    if (m_state) {
        lua_close(m_state);
    }
}

std::optional<QString> LuaState::run(const QByteArray& code, const QString& chunkName)
{
    if (!m_state) {
        return QStringLiteral("Lua state is not initialized");
    }

    const int base = lua_gettop(m_state);
    lua_pushcfunction(m_state, messageHandler);

    const QByteArray name = '=' + chunkName.toUtf8();
    int status = luaL_loadbufferx(m_state, code.constData(), code.size(), name.constData(), "t");
    if (status == LUA_OK) {
        status = lua_pcall(m_state, 0, 0, base + 1);
    }

    std::optional<QString> error;
    if (status != LUA_OK) {
        const char* msg = lua_tostring(m_state, -1);
        error = msg ? QString::fromUtf8(msg) : QStringLiteral("unknown Lua error");
    }
    lua_settop(m_state, base);
    return error;
}
