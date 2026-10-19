// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

// Lua raises errors with longjmp, which skips C++ destructors. The C functions here never keep a C++ object alive
// across a Lua call that may raise an error: C++ work happens in helpers that only use non-raising accessors, and
// anything that must survive an error lives in the SettingsStore, which the userdata's __gc frees.

#include "SettingsApi.h"

#include <QRegularExpression>
#include <QTimer>

#include <lua.hpp>

#include "settings/INISettingsObject.h"
#include "settings/Setting.h"

namespace {

constexpr const char* s_metatableName = "prism.Settings";
// Registry key of the proxy, so prism.settings can only be called once
constexpr const char* s_registryKey = "prism.settings";

// Indices of the proxy's user values
constexpr int s_defaultsIndex = 1;
constexpr int s_valuesIndex = 2;

struct SettingsStore {
    explicit SettingsStore(const QString& path) : object(path) { object.suspendSave(); }

    INISettingsObject object;
    // Holds a string while it is pushed to Lua, which may fail
    QByteArray scratch;
    bool flushScheduled = false;
};

struct SettingsUserdata {
    SettingsStore* store;
};

bool isValidName(const char* name)
{
    // "/" would create INI groups; ConfigVersion is used by INIFile itself
    static const QRegularExpression s_namePattern("^[A-Za-z_][A-Za-z0-9_]{0,63}$");
    const QString text = QString::fromUtf8(name);
    return text != "ConfigVersion" && s_namePattern.match(text).hasMatch();
}

// The value at index must be a boolean, a number or a string. Makes no Lua call that raises errors.
QVariant toVariant(lua_State* L, int index)
{
    switch (lua_type(L, index)) {
        case LUA_TBOOLEAN:
            return bool(lua_toboolean(L, index));
        case LUA_TNUMBER:
            if (lua_isinteger(L, index)) {
                return qlonglong(lua_tointeger(L, index));
            }
            return double(lua_tonumber(L, index));
        default: {
            size_t length = 0;
            const char* text = lua_tolstring(L, index, &length);
            return QString::fromUtf8(text, static_cast<qsizetype>(length));
        }
    }
}

struct StoredValue {
    bool valid = false;
    bool boolean = false;
    lua_Integer integer = 0;
    lua_Number number = 0;
};

// Registers the setting whose name and default are at -2 and -1, and reads its stored value converted to the
// default's type; strings go to store->scratch. Values that cannot be converted (e.g. after editing the file by
// hand) are not valid. Makes no Lua call that raises errors.
StoredValue registerAndRead(lua_State* L, SettingsStore* store)
{
    const QString name = QString::fromUtf8(lua_tostring(L, -2));
    store->object.registerSetting(name, toVariant(L, -1));
    const QVariant stored = store->object.get(name);

    StoredValue value;
    switch (lua_type(L, -1)) {
        case LUA_TBOOLEAN: {
            const QString text = stored.toString().toLower();
            value.valid = text == "true" || text == "false";
            value.boolean = text == "true";
            break;
        }
        case LUA_TNUMBER:
            if (lua_isinteger(L, -1)) {
                value.integer = stored.toLongLong(&value.valid);
            } else {
                value.number = stored.toDouble(&value.valid);
            }
            break;
        case LUA_TSTRING:
            store->scratch = stored.toString().toUtf8();
            value.valid = true;
            break;
    }
    return value;
}

// Stores the value at index 3 (or resets the setting) for the name at index 2. Makes no Lua call that raises errors.
void writeSetting(lua_State* L, SettingsStore* store, bool reset)
{
    std::shared_ptr<Setting> setting = store->object.getSetting(QString::fromUtf8(lua_tostring(L, 2)));
    if (reset) {
        setting->reset();
    } else {
        setting->set(toVariant(L, 3));
    }

    if (!store->flushScheduled) {
        store->flushScheduled = true;
        // Runs once the current call into Lua returns. Bound to the settings object, so it is dropped if the state
        // is closed first, which flushes anyway.
        QTimer::singleShot(0, &store->object, [store] {
            store->flushScheduled = false;
            store->object.resumeSave();
            store->object.suspendSave();
        });
    }
}

SettingsStore* checkStore(lua_State* L)
{
    return static_cast<SettingsUserdata*>(luaL_checkudata(L, 1, s_metatableName))->store;
}

void checkName(lua_State* L)
{
    if (lua_type(L, 2) != LUA_TSTRING) {
        luaL_error(L, "setting names must be strings, got %s", luaL_typename(L, 2));
    }
}

int settingsIndex(lua_State* L)
{
    checkStore(L);
    checkName(L);
    lua_getiuservalue(L, 1, s_valuesIndex);
    lua_pushvalue(L, 2);
    if (lua_rawget(L, -2) == LUA_TNIL) {
        return luaL_error(L, "unknown setting '%s'", lua_tostring(L, 2));
    }
    return 1;
}

int settingsNewIndex(lua_State* L)
{
    SettingsStore* store = checkStore(L);
    checkName(L);
    lua_settop(L, 3);
    lua_getiuservalue(L, 1, s_defaultsIndex);
    lua_pushvalue(L, 2);
    if (lua_rawget(L, 4) == LUA_TNIL) {
        return luaL_error(L, "unknown setting '%s'", lua_tostring(L, 2));
    }

    const bool reset = lua_isnil(L, 3);
    if (reset) {
        lua_pushvalue(L, 5);
        lua_replace(L, 3);
    }
    const int expected = lua_type(L, 5);
    if (lua_type(L, 3) != expected) {
        return luaL_error(L, "setting '%s' must be a %s, got %s", lua_tostring(L, 2), lua_typename(L, expected), luaL_typename(L, 3));
    }
    if (expected == LUA_TNUMBER && lua_isinteger(L, 5)) {
        int isInteger = 0;
        const lua_Integer value = lua_tointegerx(L, 3, &isInteger);
        if (!isInteger) {
            return luaL_error(L, "setting '%s' must be an integer", lua_tostring(L, 2));
        }
        lua_pushinteger(L, value);
        lua_replace(L, 3);
    }
    if (expected == LUA_TSTRING && lua_rawlen(L, 3) > PrismApi::s_maxSettingStringLength) {
        return luaL_error(L, "setting '%s' is longer than %d bytes", lua_tostring(L, 2), int(PrismApi::s_maxSettingStringLength));
    }

    // Update the Lua side first: it may fail, and then nothing has changed
    lua_getiuservalue(L, 1, s_valuesIndex);
    lua_pushvalue(L, 2);
    lua_pushvalue(L, 3);
    lua_rawset(L, -3);

    writeSetting(L, store, reset);
    return 0;
}

int settingsGc(lua_State* L)
{
    auto* userdata = static_cast<SettingsUserdata*>(lua_touserdata(L, 1));
    if (userdata->store) {
        userdata->store->object.resumeSave();
        delete userdata->store;
        userdata->store = nullptr;
    }
    return 0;
}

// prism.settings(defaults): upvalue 1 is the INI file path
int createSettings(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 1);
    if (lua_getfield(L, LUA_REGISTRYINDEX, s_registryKey) != LUA_TNIL) {
        return luaL_error(L, "prism.settings can only be called once");
    }
    lua_pop(L, 1);

    // Validated copy of the defaults, so later changes to the argument have no effect
    lua_newtable(L);
    const int defaults = lua_gettop(L);
    int count = 0;
    lua_pushnil(L);
    while (lua_next(L, 1) != 0) {
        if (lua_type(L, -2) != LUA_TSTRING) {
            return luaL_error(L, "setting names must be strings, got %s", luaL_typename(L, -2));
        }
        if (!isValidName(lua_tostring(L, -2))) {
            return luaL_error(L, "invalid setting name '%s'", lua_tostring(L, -2));
        }
        const int type = lua_type(L, -1);
        if (type != LUA_TBOOLEAN && type != LUA_TNUMBER && type != LUA_TSTRING) {
            return luaL_error(L, "default of setting '%s' must be a boolean, number or string", lua_tostring(L, -2));
        }
        if (type == LUA_TSTRING && lua_rawlen(L, -1) > PrismApi::s_maxSettingStringLength) {
            return luaL_error(L, "default of setting '%s' is longer than %d bytes", lua_tostring(L, -2),
                              int(PrismApi::s_maxSettingStringLength));
        }
        if (++count > PrismApi::s_maxSettings) {
            return luaL_error(L, "too many settings, the maximum is %d", PrismApi::s_maxSettings);
        }
        lua_pushvalue(L, -2);
        lua_insert(L, -2);
        lua_rawset(L, defaults);
    }

    lua_newtable(L);
    const int values = lua_gettop(L);

    // Created before the store, so __gc frees it if anything below fails
    auto* userdata = static_cast<SettingsUserdata*>(lua_newuserdatauv(L, sizeof(SettingsUserdata), 2));
    userdata->store = nullptr;
    luaL_setmetatable(L, s_metatableName);
    const int proxy = lua_gettop(L);
    lua_pushvalue(L, defaults);
    lua_setiuservalue(L, proxy, s_defaultsIndex);
    lua_pushvalue(L, values);
    lua_setiuservalue(L, proxy, s_valuesIndex);

    userdata->store = new SettingsStore(QString::fromUtf8(lua_tostring(L, lua_upvalueindex(1))));
    SettingsStore* store = userdata->store;

    lua_pushnil(L);
    while (lua_next(L, defaults) != 0) {
        // Stack: name, default
        const StoredValue stored = registerAndRead(L, store);
        if (!stored.valid) {
            lua_pushvalue(L, -1);
        } else if (lua_type(L, -1) == LUA_TBOOLEAN) {
            lua_pushboolean(L, stored.boolean);
        } else if (lua_type(L, -1) == LUA_TSTRING) {
            lua_pushlstring(L, store->scratch.constData(), store->scratch.size());
        } else if (lua_isinteger(L, -1)) {
            lua_pushinteger(L, stored.integer);
        } else {
            lua_pushnumber(L, stored.number);
        }
        // Stack: name, default, value
        lua_remove(L, -2);
        lua_pushvalue(L, -2);
        lua_insert(L, -2);
        lua_rawset(L, values);
    }
    store->scratch.clear();

    lua_pushvalue(L, proxy);
    lua_setfield(L, LUA_REGISTRYINDEX, s_registryKey);
    return 1;
}

}  // namespace

namespace PrismApi {

void pushSettingsFunction(lua_State* L, const QByteArray& path)
{
    if (luaL_newmetatable(L, s_metatableName)) {
        lua_pushcfunction(L, settingsIndex);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, settingsNewIndex);
        lua_setfield(L, -2, "__newindex");
        lua_pushcfunction(L, settingsGc);
        lua_setfield(L, -2, "__gc");
        // Hide the metatable from scripts
        lua_pushboolean(L, false);
        lua_setfield(L, -2, "__metatable");
    }
    lua_pop(L, 1);

    lua_pushlstring(L, path.constData(), path.size());
    lua_pushcclosure(L, createSettings, 1);
}

}  // namespace PrismApi
