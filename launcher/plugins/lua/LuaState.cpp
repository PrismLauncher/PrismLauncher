// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "LuaState.h"

#include <QDebug>

#include <algorithm>
#include <cstdlib>

#include <lua.hpp>

namespace {

// Instructions between two calls to the instruction hook
constexpr int s_hookInterval = 1000;

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

// Wraps the original `setmetatable` (upvalue 1) rejecting __gc: finalizers run with hooks disabled, so the instruction
// limit could not stop them. Objects are only marked for finalization here, adding __gc later has no effect.
int setmetatableWithoutGc(lua_State* L)
{
    if (lua_type(L, 2) == LUA_TTABLE) {
        lua_pushliteral(L, "__gc");
        if (lua_rawget(L, 2) != LUA_TNIL) {
            return luaL_error(L, "__gc metamethods are not allowed");
        }
        lua_pop(L, 1);
    }
    const int nargs = lua_gettop(L);
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_insert(L, 1);
    lua_call(L, nargs, 1);
    return 1;
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

    lua_getglobal(L, "setmetatable");
    lua_pushcclosure(L, setmetatableWithoutGc, 1);
    lua_setglobal(L, "setmetatable");
    return 0;
}

int openUnrestrictedLibs(lua_State* L)
{
    // Not debug: debug.sethook would turn the instruction watchdog off
    luaL_openselectedlibs(L, LUA_IOLIBK | LUA_OSLIBK | LUA_LOADLIBK, 0);
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

int panic(lua_State* L)
{
    const char* msg = lua_type(L, -1) == LUA_TSTRING ? lua_tostring(L, -1) : "error object is not a string";
    qCritical() << "Unprotected error in call to Lua API:" << msg;
    return 0;  // Lua aborts after this
}

}  // namespace

LuaState::LuaState(std::size_t memoryLimit) : m_memory{ 0, memoryLimit }
{
    m_state = lua_newstate(allocate, &m_memory, luaL_makeseed(nullptr));
    if (!m_state) {
        qWarning() << "Failed to create Lua state";
        return;
    }
    // Coroutines copy the main thread's extra space, so the hook can always find this object
    *static_cast<LuaState**>(lua_getextraspace(m_state)) = this;
    lua_atpanic(m_state, panic);

    lua_pushcfunction(m_state, openSafeLibs);
    if (lua_pcall(m_state, 0, 0, 0) != LUA_OK) {
        qWarning() << "Failed to open Lua libraries:" << lua_tostring(m_state, -1);
        lua_close(m_state);
        m_state = nullptr;
    }
}

LuaState::~LuaState()
{
    // Expired first: closing the state may destroy objects (e.g. windows) whose callbacks must not reach it anymore
    m_lifetime.reset();
    if (m_state) {
        lua_close(m_state);
    }
    Q_ASSERT(m_memory.used == 0);
}

std::optional<QString> LuaState::openUnrestrictedLibraries()
{
    if (!m_state) {
        return QStringLiteral("Lua state is not initialized");
    }
    lua_pushcfunction(m_state, openUnrestrictedLibs);
    if (lua_pcall(m_state, 0, 0, 0) != LUA_OK) {
        const char* msg = lua_tostring(m_state, -1);
        std::optional<QString> error = msg ? QString::fromUtf8(msg) : QStringLiteral("unknown Lua error");
        lua_pop(m_state, 1);
        return error;
    }
    return std::nullopt;
}

LuaState* LuaState::fromLua(lua_State* L)
{
    return *static_cast<LuaState**>(lua_getextraspace(L));
}

void* LuaState::allocate(void* ud, void* ptr, std::size_t osize, std::size_t nsize)
{
    auto* memory = static_cast<Memory*>(ud);
    // When ptr is null, osize encodes the type of the new object instead of a size
    const std::size_t oldSize = ptr ? osize : 0;

    if (nsize == 0) {
        std::free(ptr);
        memory->used -= oldSize;
        return nullptr;
    }

    // Only growth is checked against the limit
    if (nsize > oldSize && nsize - oldSize > memory->limit - memory->used) {
        return nullptr;
    }

    void* newPtr = std::realloc(ptr, nsize);
    if (newPtr) {
        memory->used = memory->used - oldSize + nsize;
    }
    return newPtr;
}

void LuaState::instructionHook(lua_State* L, lua_Debug*)
{
    auto* self = *static_cast<LuaState**>(lua_getextraspace(L));
    self->m_instructionsUsed += lua_gethookcount(L);
    if (self->m_instructionsUsed > self->m_instructionLimit) {
        // From now on fail on every instruction, so a pcall in the script cannot catch the error and keep looping
        lua_sethook(L, instructionHook, LUA_MASKCOUNT, 1);
        luaL_error(L, "instruction limit exceeded");
    }
}

void LuaState::resetInstructionBudget()
{
    m_instructionsUsed = 0;
    // Coroutines inherit the hook when created, so only the main thread needs it
    lua_sethook(m_state, instructionHook, LUA_MASKCOUNT, s_hookInterval);
}

std::optional<QString> LuaState::run(const QByteArray& code, const QString& chunkName)
{
    if (!m_state) {
        return QStringLiteral("Lua state is not initialized");
    }

    const QByteArray name = '=' + chunkName.toUtf8();
    if (luaL_loadbufferx(m_state, code.constData(), code.size(), name.constData(), "t") != LUA_OK) {
        const char* msg = lua_tostring(m_state, -1);
        std::optional<QString> error = msg ? QString::fromUtf8(msg) : QStringLiteral("unknown Lua error");
        lua_pop(m_state, 1);
        return error;
    }
    return call(0);
}

std::optional<QString> LuaState::call(int nargs)
{
    if (!m_state) {
        return QStringLiteral("Lua state is not initialized");
    }

    if (m_callDepth == 0) {
        resetInstructionBudget();
    }
    const int base = lua_gettop(m_state) - nargs - 1;
    lua_pushcfunction(m_state, messageHandler);
    lua_insert(m_state, base + 1);
    m_callDepth++;
    const int status = lua_pcall(m_state, nargs, 0, base + 1);
    m_callDepth--;

    std::optional<QString> error;
    if (status != LUA_OK) {
        const char* msg = lua_tostring(m_state, -1);
        error = msg ? QString::fromUtf8(msg) : QStringLiteral("unknown Lua error");
    }
    lua_settop(m_state, base);
    return error;
}
