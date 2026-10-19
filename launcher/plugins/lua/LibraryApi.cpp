// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

// Lua raises errors with longjmp, which skips C++ destructors. The C functions here only hold plain pointers across
// Lua calls that may raise errors: files are read into static buffers (plugins only run on the main thread).

#include "LibraryApi.h"

#include <QDir>
#include <QFile>
#include <QRegularExpression>

#include <lua.hpp>

#include "plugins/lua/Prelude.h"

namespace {

// Registry key of the table of loaded modules; false marks one being loaded
constexpr const char* s_modulesKey = "prism.modules";

QByteArray s_code;
QByteArray s_chunkName;
QByteArray s_error;

bool isValidModuleName(const char* name)
{
    static const QRegularExpression s_pattern("^[A-Za-z0-9_]+(\\.[A-Za-z0-9_]+)*$");
    return s_pattern.match(QString::fromUtf8(name)).hasMatch();
}

// Reads the module into s_code; its name is already valid, so it stays inside the plugin's folder
bool readModule(const char* pluginDir, const char* name)
{
    const QString relative = QString::fromUtf8(name).replace('.', '/');
    const QDir dir(QString::fromUtf8(pluginDir));
    for (const QString& candidate : { relative + ".lua", relative + "/init.lua" }) {
        QFile file(dir.filePath(candidate));
        if (!file.exists()) {
            continue;
        }
        if (!file.open(QIODevice::ReadOnly)) {
            s_error = QString("could not read %1: %2").arg(candidate, file.errorString()).toUtf8();
            return false;
        }
        s_code = file.read(PrismApi::s_maxModuleSize + 1);
        if (s_code.size() > PrismApi::s_maxModuleSize) {
            s_error = QString("%1 is larger than %2 bytes").arg(candidate).arg(PrismApi::s_maxModuleSize).toUtf8();
            return false;
        }
        s_chunkName = '=' + candidate.toUtf8();
        return true;
    }
    s_error = QString("module '%1' not found: no %2.lua or %2/init.lua in the plugin").arg(QString::fromUtf8(name), relative).toUtf8();
    return false;
}

// require(name): upvalue 1 is the plugin's folder
int require(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    lua_settop(L, 1);
    lua_getfield(L, LUA_REGISTRYINDEX, s_modulesKey);
    const int modules = lua_gettop(L);
    const int cached = lua_getfield(L, modules, name);
    if (cached == LUA_TBOOLEAN && !lua_toboolean(L, -1)) {
        return luaL_error(L, "module '%s' requires itself", name);
    }
    if (cached != LUA_TNIL) {
        return 1;
    }
    lua_pop(L, 1);

    if (!isValidModuleName(name)) {
        return luaL_error(L, "invalid module name '%s'", name);
    }
    if (!readModule(lua_tostring(L, lua_upvalueindex(1)), name)) {
        lua_pushlstring(L, s_error.constData(), s_error.size());
        return lua_error(L);
    }
    const int status = luaL_loadbufferx(L, s_code.constData(), s_code.size(), s_chunkName.constData(), "t");
    s_code.clear();
    if (status != LUA_OK) {
        return lua_error(L);
    }

    lua_pushboolean(L, false);
    lua_setfield(L, modules, name);
    lua_pushvalue(L, 1);
    if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
        // Forget it, so a later require tries again
        lua_pushnil(L);
        lua_setfield(L, modules, name);
        return lua_error(L);
    }
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_pushboolean(L, true);
    }
    lua_pushvalue(L, -1);
    lua_setfield(L, modules, name);
    return 1;
}

}  // namespace

namespace PrismApi {

void installLibrary(lua_State* L, const QByteArray& pluginDir)
{
    const int prism = lua_gettop(L);

    // Unrestricted plugins already have the real require, set up for the plugin's folder
    if (lua_getglobal(L, "package") != LUA_TTABLE) {
        lua_newtable(L);
        lua_setfield(L, LUA_REGISTRYINDEX, s_modulesKey);
        lua_pushlstring(L, pluginDir.constData(), pluginDir.size());
        lua_pushcclosure(L, require, 1);
        lua_setglobal(L, "require");
    }
    lua_pop(L, 1);

    if (luaL_loadbufferx(L, reinterpret_cast<const char*>(s_prelude), sizeof(s_prelude) - 1, "=prelude", "t") != LUA_OK) {
        lua_error(L);
    }
    lua_pushvalue(L, prism);
    lua_call(L, 1, 0);
}

}  // namespace PrismApi
