// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

// Lua raises errors with longjmp, which skips C++ destructors. The C functions here never keep a C++ object alive
// across a Lua call that may raise an error (anything that allocates, calls metamethods or is luaL_check*/luaL_error).

#include "PrismApi.h"

#include <QDir>
#include <QFileInfo>

#include <algorithm>
#include <array>
#include <cstring>

#include <lua.hpp>

#include "launch/LaunchStepIds.h"
#include "plugins/Logging.h"
#include "plugins/PluginManifest.h"
#include "plugins/lua/FsApi.h"
#include "plugins/lua/HttpApi.h"
#include "plugins/lua/LibraryApi.h"
#include "plugins/lua/LuaState.h"
#include "plugins/lua/SettingsApi.h"
#include "plugins/lua/UiApi.h"

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

// Registry key of the table that maps event names to arrays of handler entries ({ fn = function, instance = id })
constexpr const char* s_handlersKey = "prism.handlers";

struct EventInfo {
    const char* name;
    // Needed to subscribe, or nullptr
    const char* permission;
};
constexpr std::array s_events = {
    EventInfo{ "launcher.ready", nullptr },
    EventInfo{ "instance.created", "instances.read" },
    EventInfo{ "instance.removed", "instances.read" },
    EventInfo{ "instance.status", "instances.read" },
    EventInfo{ "launch.step", "instances.read" },
    EventInfo{ "launch.finished", "instances.read" },
    EventInfo{ "game.log", "game.log" },
};

const EventInfo* findEvent(const char* name)
{
    auto it = std::find_if(s_events.begin(), s_events.end(), [name](const EventInfo& info) { return std::strcmp(name, info.name) == 0; });
    return it != s_events.end() ? &*it : nullptr;
}

// prism.on(event, [opts], handler): upvalue 1 is the set of the plugin's permissions.
// opts.instance limits the handler to the events of that instance.
int on(lua_State* L)
{
    const char* event = luaL_checkstring(L, 1);
    const int opts = lua_istable(L, 2) ? 2 : 0;
    const int handler = opts ? 3 : 2;
    luaL_checktype(L, handler, LUA_TFUNCTION);

    const EventInfo* info = findEvent(event);
    if (!info) {
        return luaL_error(L, "unknown event '%s'", event);
    }
    if (info->permission) {
        lua_getfield(L, lua_upvalueindex(1), info->permission);
        if (!lua_toboolean(L, -1)) {
            return luaL_error(L, "event '%s' requires the '%s' permission", event, info->permission);
        }
        lua_pop(L, 1);
    }

    lua_createtable(L, 0, 2);
    const int entry = lua_gettop(L);
    lua_pushvalue(L, handler);
    lua_setfield(L, entry, "fn");
    if (opts) {
        const int type = lua_getfield(L, opts, "instance");
        if (type != LUA_TNIL && type != LUA_TSTRING) {
            return luaL_error(L, "'instance' must be an instance id");
        }
        lua_setfield(L, entry, "instance");
    }

    lua_getfield(L, LUA_REGISTRYINDEX, s_handlersKey);
    if (lua_getfield(L, -1, event) == LUA_TNIL) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, -3, event);
    }
    lua_pushvalue(L, entry);
    lua_rawseti(L, -2, static_cast<lua_Integer>(lua_rawlen(L, -2)) + 1);
    return 0;
}

struct DispatchArgs {
    const char* event;
    const PrismApi::EventFields* fields;
};

// The "id" field of the event, used to filter handlers by instance
const QByteArray* eventInstanceId(const PrismApi::EventFields& fields)
{
    for (const auto& field : fields) {
        if (std::strcmp(field.key, "id") == 0) {
            return std::get_if<QByteArray>(&field.value);
        }
    }
    return nullptr;
}

void pushEventTable(lua_State* L, const PrismApi::EventFields& fields)
{
    lua_createtable(L, 0, static_cast<int>(fields.size()));
    for (const auto& field : fields) {
        if (const auto* text = std::get_if<QByteArray>(&field.value)) {
            lua_pushlstring(L, text->constData(), text->size());
        } else if (const auto* flag = std::get_if<bool>(&field.value)) {
            lua_pushboolean(L, *flag);
        } else {
            lua_pushinteger(L, std::get<qint64>(field.value));
        }
        lua_setfield(L, -2, field.key);
    }
}

// Calls the handlers of an event. Argument 1 is a light userdata pointing to the DispatchArgs.
int dispatch(lua_State* L)
{
    const auto* args = static_cast<const DispatchArgs*>(lua_touserdata(L, 1));
    const QByteArray* instanceId = eventInstanceId(*args->fields);
    lua_getfield(L, LUA_REGISTRYINDEX, s_handlersKey);
    if (lua_getfield(L, -1, args->event) != LUA_TTABLE) {
        return 0;
    }
    // Handlers registered meanwhile only run from the next event on
    const auto count = static_cast<lua_Integer>(lua_rawlen(L, -1));
    for (lua_Integer i = 1; i <= count; i++) {
        lua_rawgeti(L, -1, i);
        if (lua_getfield(L, -1, "instance") == LUA_TSTRING &&
            (!instanceId || std::strcmp(lua_tostring(L, -1), instanceId->constData()) != 0)) {
            lua_pop(L, 2);
            continue;
        }
        lua_pop(L, 1);
        lua_getfield(L, -1, "fn");
        // A fresh table for each handler, so one cannot change what the next one sees
        pushEventTable(L, *args->fields);
        lua_call(L, 1, 0);
        lua_pop(L, 1);
    }
    return 0;
}

// Registry key of the array of steps registered with prism.launch.addStep
constexpr const char* s_launchStepsKey = "prism.launchSteps";
// Registry key (by address) of the box holding the LaunchContext of the running step
const char s_contextKey = 0;

bool isLaunchPhase(const char* id)
{
    return std::any_of(LaunchStepIds::all.begin(), LaunchStepIds::all.end(),
                       [id](const char* phase) { return std::strcmp(id, phase) == 0; });
}

// prism.launch.addStep{ before = phase | after = phase, run = function(ctx), name = string }: upvalue 1 tells whether
// the plugin has the launch.hooks permission
int addLaunchStep(lua_State* L)
{
    if (!lua_toboolean(L, lua_upvalueindex(1))) {
        return luaL_error(L, "prism.launch.addStep requires the 'launch.hooks' permission");
    }
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 1);

    const int beforeType = lua_getfield(L, 1, "before");
    const int afterType = lua_getfield(L, 1, "after");
    if ((beforeType == LUA_TNIL) == (afterType == LUA_TNIL)) {
        return luaL_error(L, "set either 'before' or 'after' to a launch phase");
    }
    const bool before = beforeType != LUA_TNIL;
    const int anchor = before ? 2 : 3;
    if (lua_type(L, anchor) != LUA_TSTRING || !isLaunchPhase(lua_tostring(L, anchor))) {
        return luaL_error(L, "unknown launch phase '%s'", luaL_tolstring(L, anchor, nullptr));
    }
    if (lua_getfield(L, 1, "run") != LUA_TFUNCTION) {
        return luaL_error(L, "'run' must be a function");
    }
    const int nameType = lua_getfield(L, 1, "name");
    if (nameType != LUA_TNIL && nameType != LUA_TSTRING) {
        return luaL_error(L, "'name' must be a string");
    }
    // Stack: spec, before, after, run, name

    lua_getfield(L, LUA_REGISTRYINDEX, s_launchStepsKey);
    const auto count = static_cast<lua_Integer>(lua_rawlen(L, -1));
    if (count >= PrismApi::s_maxLaunchSteps) {
        return luaL_error(L, "too many launch steps, the maximum is %d", PrismApi::s_maxLaunchSteps);
    }
    lua_createtable(L, 0, 4);
    lua_pushvalue(L, anchor);
    lua_setfield(L, -2, "anchor");
    lua_pushboolean(L, before);
    lua_setfield(L, -2, "before");
    lua_pushvalue(L, 4);
    lua_setfield(L, -2, "run");
    lua_pushvalue(L, 5);
    lua_setfield(L, -2, "name");
    lua_rawseti(L, -2, count + 1);
    return 0;
}

// Argument 1 is a light userdata pointing to the QList<LaunchStepInfo> to fill
int collectLaunchSteps(lua_State* L)
{
    auto* steps = static_cast<QList<PrismApi::LaunchStepInfo>*>(lua_touserdata(L, 1));
    lua_getfield(L, LUA_REGISTRYINDEX, s_launchStepsKey);
    const auto count = static_cast<int>(lua_rawlen(L, -1));
    for (int i = 1; i <= count; i++) {
        lua_rawgeti(L, -1, i);
        lua_getfield(L, -1, "anchor");
        lua_getfield(L, -2, "before");
        lua_getfield(L, -3, "name");
        // Only non-raising accessors while C++ temporaries exist
        steps->append({ i, QString::fromUtf8(lua_tostring(L, -3)), bool(lua_toboolean(L, -2)),
                        lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString() });
        lua_pop(L, 4);
    }
    return 0;
}

PrismApi::LaunchContext* checkContext(lua_State* L)
{
    auto* context = *static_cast<PrismApi::LaunchContext**>(lua_touserdata(L, lua_upvalueindex(1)));
    if (!context) {
        luaL_error(L, "this launch step has already finished");
    }
    return context;
}

// ctx.log(text): upvalue 1 is the context box
int contextLog(lua_State* L)
{
    PrismApi::LaunchContext* context = checkContext(L);
    size_t length = 0;
    const char* text = luaL_checklstring(L, 1, &length);
    length = std::min<size_t>(length, PrismApi::s_maxLogMessageLength);
    context->log(QString::fromUtf8(text, static_cast<qsizetype>(length)));
    return 0;
}

// ctx.abort([reason]): upvalue 1 is the context box
int contextAbort(lua_State* L)
{
    PrismApi::LaunchContext* context = checkContext(L);
    size_t length = 0;
    const char* reason = luaL_optlstring(L, 1, "aborted by plugin", &length);
    length = std::min<size_t>(length, PrismApi::s_maxLogMessageLength);
    context->aborted = true;
    context->abortReason = QString::fromUtf8(reason, static_cast<qsizetype>(length));
    // Stop the step; even if the script catches this, the launch is aborted
    return luaL_error(L, "launch aborted: %s", reason);
}

struct RunStepArgs {
    int index;
    PrismApi::LaunchContext* context;
};

// Argument 1 is a light userdata pointing to the RunStepArgs
int runStep(lua_State* L)
{
    const auto* args = static_cast<const RunStepArgs*>(lua_touserdata(L, 1));
    lua_getfield(L, LUA_REGISTRYINDEX, s_launchStepsKey);
    if (lua_rawgeti(L, -1, args->index) != LUA_TTABLE) {
        return luaL_error(L, "unknown launch step %d", args->index);
    }
    lua_getfield(L, -1, "run");

    // ctx
    lua_createtable(L, 0, 3);
    lua_createtable(L, 0, 2);
    lua_pushlstring(L, args->context->instanceId.constData(), args->context->instanceId.size());
    lua_setfield(L, -2, "id");
    lua_pushlstring(L, args->context->instanceName.constData(), args->context->instanceName.size());
    lua_setfield(L, -2, "name");
    lua_setfield(L, -2, "instance");

    // The box is cleared once the step returns, so a ctx kept by the script cannot reach a dead context
    auto* box = static_cast<PrismApi::LaunchContext**>(lua_newuserdatauv(L, sizeof(PrismApi::LaunchContext*), 0));
    *box = args->context;
    lua_pushvalue(L, -1);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &s_contextKey);
    lua_pushvalue(L, -1);
    lua_pushcclosure(L, contextLog, 1);
    lua_setfield(L, -3, "log");
    lua_pushcclosure(L, contextAbort, 1);
    lua_setfield(L, -2, "abort");

    lua_call(L, 1, 0);
    return 0;
}

void pushInstance(lua_State* L, const PrismApi::InstanceInfo& info)
{
    lua_createtable(L, 0, 6);
    lua_pushlstring(L, info.id.constData(), info.id.size());
    lua_setfield(L, -2, "id");
    lua_pushlstring(L, info.name.constData(), info.name.size());
    lua_setfield(L, -2, "name");
    lua_pushlstring(L, info.group.constData(), info.group.size());
    lua_setfield(L, -2, "group");
    lua_pushboolean(L, info.running);
    lua_setfield(L, -2, "running");
    lua_pushinteger(L, info.lastLaunch);
    lua_setfield(L, -2, "lastLaunch");
    lua_pushinteger(L, info.playTime);
    lua_setfield(L, -2, "playTime");
}

// Upvalue 1 tells whether the plugin has the instances.read permission, upvalue 2 is a light userdata pointing to
// the InstanceSource (or null)
PrismApi::InstanceSource* checkInstanceSource(lua_State* L)
{
    if (!lua_toboolean(L, lua_upvalueindex(1))) {
        luaL_error(L, "prism.instances requires the 'instances.read' permission");
    }
    return static_cast<PrismApi::InstanceSource*>(lua_touserdata(L, lua_upvalueindex(2)));
}

// prism.instances.list()
int listInstances(lua_State* L)
{
    PrismApi::InstanceSource* source = checkInstanceSource(L);
    if (!source) {
        lua_newtable(L);
        return 1;
    }
    source->scratch = source->instances();
    lua_createtable(L, static_cast<int>(source->scratch.size()), 0);
    for (qsizetype i = 0; i < source->scratch.size(); i++) {
        pushInstance(L, source->scratch.at(i));
        lua_rawseti(L, -2, i + 1);
    }
    source->scratch.clear();
    return 1;
}

// prism.instances.get(id): nil if there is no such instance
int getInstance(lua_State* L)
{
    PrismApi::InstanceSource* source = checkInstanceSource(L);
    size_t length = 0;
    const char* id = luaL_checklstring(L, 1, &length);
    if (!source) {
        lua_pushnil(L);
        return 1;
    }
    source->scratch = source->instances();
    // A view has no destructor, so it may stay alive while Lua calls raise errors
    const QByteArrayView wanted(id, static_cast<qsizetype>(length));
    source->scratch.removeIf([&wanted](const PrismApi::InstanceInfo& info) { return info.id != wanted; });
    if (source->scratch.isEmpty()) {
        lua_pushnil(L);
    } else {
        pushInstance(L, source->scratch.first());
    }
    source->scratch.clear();
    return 1;
}

// Registry key of the array of actions registered with prism.ui.addAction
constexpr const char* s_uiActionsKey = "prism.uiActions";

// prism.ui.addAction{ text = string, scope = "instance" | "launcher", run = function(ctx) }: upvalue 1 tells whether
// the plugin has the ui permission
int addUiAction(lua_State* L)
{
    if (!lua_toboolean(L, lua_upvalueindex(1))) {
        return luaL_error(L, "prism.ui.addAction requires the 'ui' permission");
    }
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 1);

    if (lua_getfield(L, 1, "text") != LUA_TSTRING || lua_rawlen(L, 2) == 0) {
        return luaL_error(L, "'text' must be a non-empty string");
    }
    if (lua_rawlen(L, 2) > static_cast<size_t>(PrismApi::s_maxUiActionTextLength)) {
        return luaL_error(L, "'text' is longer than %d bytes", int(PrismApi::s_maxUiActionTextLength));
    }
    if (lua_getfield(L, 1, "scope") != LUA_TSTRING ||
        (std::strcmp(lua_tostring(L, 3), "instance") != 0 && std::strcmp(lua_tostring(L, 3), "launcher") != 0)) {
        return luaL_error(L, "'scope' must be \"instance\" or \"launcher\"");
    }
    if (lua_getfield(L, 1, "run") != LUA_TFUNCTION) {
        return luaL_error(L, "'run' must be a function");
    }
    // Stack: spec, text, scope, run

    lua_getfield(L, LUA_REGISTRYINDEX, s_uiActionsKey);
    const auto count = static_cast<lua_Integer>(lua_rawlen(L, -1));
    if (count >= PrismApi::s_maxUiActions) {
        return luaL_error(L, "too many actions, the maximum is %d", PrismApi::s_maxUiActions);
    }
    lua_createtable(L, 0, 3);
    lua_pushvalue(L, 2);
    lua_setfield(L, -2, "text");
    lua_pushvalue(L, 3);
    lua_setfield(L, -2, "scope");
    lua_pushvalue(L, 4);
    lua_setfield(L, -2, "run");
    lua_rawseti(L, -2, count + 1);
    return 0;
}

// Argument 1 is a light userdata pointing to the QList<UiActionInfo> to fill
int collectUiActions(lua_State* L)
{
    auto* actions = static_cast<QList<PrismApi::UiActionInfo>*>(lua_touserdata(L, 1));
    lua_getfield(L, LUA_REGISTRYINDEX, s_uiActionsKey);
    const auto count = static_cast<int>(lua_rawlen(L, -1));
    for (int i = 1; i <= count; i++) {
        lua_rawgeti(L, -1, i);
        lua_getfield(L, -1, "scope");
        lua_getfield(L, -2, "text");
        // Only non-raising accessors while C++ temporaries exist
        const auto scope =
            std::strcmp(lua_tostring(L, -2), "instance") == 0 ? PrismApi::ActionScope::Instance : PrismApi::ActionScope::Launcher;
        actions->append({ i, scope, QString::fromUtf8(lua_tostring(L, -1)) });
        lua_pop(L, 3);
    }
    return 0;
}

struct RunActionArgs {
    int index;
    const PrismApi::InstanceInfo* instance;
};

// Argument 1 is a light userdata pointing to the RunActionArgs
int runAction(lua_State* L)
{
    const auto* args = static_cast<const RunActionArgs*>(lua_touserdata(L, 1));
    lua_getfield(L, LUA_REGISTRYINDEX, s_uiActionsKey);
    if (lua_rawgeti(L, -1, args->index) != LUA_TTABLE) {
        return luaL_error(L, "unknown action %d", args->index);
    }
    lua_getfield(L, -1, "run");
    lua_createtable(L, 0, 1);  // ctx
    if (args->instance) {
        pushInstance(L, *args->instance);
        lua_setfield(L, -2, "instance");
    }
    lua_call(L, 1, 0);
    return 0;
}

// prism.ui.notify(text, [level]): upvalue 1 tells whether the plugin has the ui permission, upvalue 2 is a light
// userdata pointing to the Notifier (or null), upvalue 3 is the plugin id
int notify(lua_State* L)
{
    if (!lua_toboolean(L, lua_upvalueindex(1))) {
        return luaL_error(L, "prism.ui.notify requires the 'ui' permission");
    }
    size_t length = 0;
    const char* text = luaL_checklstring(L, 1, &length);
    const char* levelName = luaL_optstring(L, 2, "info");
    PrismApi::NotifyLevel level;
    if (std::strcmp(levelName, "info") == 0) {
        level = PrismApi::NotifyLevel::Info;
    } else if (std::strcmp(levelName, "warning") == 0) {
        level = PrismApi::NotifyLevel::Warning;
    } else if (std::strcmp(levelName, "error") == 0) {
        level = PrismApi::NotifyLevel::Error;
    } else {
        return luaL_error(L, "level must be \"info\", \"warning\" or \"error\"");
    }
    length = std::min<size_t>(length, PrismApi::s_maxNotificationLength);

    size_t idLength = 0;
    const char* id = lua_tolstring(L, lua_upvalueindex(3), &idLength);
    const QString pluginId = QString::fromUtf8(id, static_cast<qsizetype>(idLength));
    const QString message = QString::fromUtf8(text, static_cast<qsizetype>(length));
    // No Lua calls from here on, so the strings above are always destroyed
    if (auto* notifier = static_cast<PrismApi::Notifier*>(lua_touserdata(L, lua_upvalueindex(2)))) {
        notifier->notify(pluginId, message, level);
    } else {
        qCInfo(pluginLogC).noquote() << QString("[%1] notification: %2").arg(pluginId, message);
    }
    return 0;
}

struct InstallArgs {
    QByteArray id;
    QByteArray settingsPath;
    bool launchHooks;
    QList<QByteArray> permissions;
    bool instancesRead;
    PrismApi::InstanceSource* instances;
    bool ui;
    PrismApi::Notifier* notifier;
    QByteArray dataDir;
    bool fsInstance;
    bool net;
    PrismApi::HttpClient* http;
    // Where require looks for modules, when package is available
    QByteArray packagePath;
    QByteArray packageCPath;
    QByteArray pluginDir;
    QByteArray pluginsDir;
    bool fsPlugins;
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
    lua_createtable(L, 0, static_cast<int>(args->permissions.size()));
    for (const QByteArray& permission : args->permissions) {
        lua_pushboolean(L, true);
        lua_setfield(L, -2, permission.constData());
    }
    lua_pushcclosure(L, on, 1);
    lua_setfield(L, -2, "on");

    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, s_launchStepsKey);
    lua_createtable(L, 0, 1);  // prism.launch
    lua_pushboolean(L, args->launchHooks);
    lua_pushcclosure(L, addLaunchStep, 1);
    lua_setfield(L, -2, "addStep");
    lua_setfield(L, -2, "launch");

    lua_createtable(L, 0, 2);  // prism.instances
    lua_pushboolean(L, args->instancesRead);
    lua_pushlightuserdata(L, args->instances);
    lua_pushcclosure(L, listInstances, 2);
    lua_setfield(L, -2, "list");
    lua_pushboolean(L, args->instancesRead);
    lua_pushlightuserdata(L, args->instances);
    lua_pushcclosure(L, getInstance, 2);
    lua_setfield(L, -2, "get");
    lua_setfield(L, -2, "instances");

    PrismApi::pushFsTable(L, args->dataDir, args->fsInstance, args->instances, args->pluginsDir, args->fsPlugins);
    lua_setfield(L, -2, "fs");

    PrismApi::setUpTasks(L, args->id);
    PrismApi::pushAsyncFunction(L);
    lua_setfield(L, -2, "async");
    PrismApi::pushHttpTable(L, args->net, args->http);
    lua_setfield(L, -2, "http");

    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, s_uiActionsKey);
    lua_createtable(L, 0, 1);  // prism.ui
    lua_pushboolean(L, args->ui);
    lua_pushcclosure(L, addUiAction, 1);
    lua_setfield(L, -2, "addAction");
    lua_pushboolean(L, args->ui);
    lua_pushlightuserdata(L, args->notifier);
    lua_pushlstring(L, args->id.constData(), args->id.size());
    lua_pushcclosure(L, notify, 3);
    lua_setfield(L, -2, "notify");
    PrismApi::addWindowFunction(L, args->ui);
    lua_setfield(L, -2, "ui");

    PrismApi::installLibrary(L, args->pluginDir);
    lua_setglobal(L, "prism");

    // Only unrestricted plugins have package: let require load the plugin's own modules
    if (lua_getglobal(L, "package") == LUA_TTABLE) {
        lua_pushlstring(L, args->packagePath.constData(), args->packagePath.size());
        lua_setfield(L, -2, "path");
        lua_pushlstring(L, args->packageCPath.constData(), args->packageCPath.size());
        lua_setfield(L, -2, "cpath");
    }
    lua_pop(L, 1);
    return 0;
}

}  // namespace

namespace PrismApi {

std::optional<QString> install(LuaState& state, const PluginManifest& manifest, const QString& directory, const Host& host)
{
    // Everything that allocates Lua memory happens inside the protected call, so running out of memory cannot panic
    InstallArgs args{ manifest.id.toUtf8(),
                      QDir(directory).filePath("settings.ini").toUtf8(),
                      manifest.hasPermission("launch.hooks"),
                      {},
                      manifest.hasPermission("instances.read"),
                      host.instances,
                      manifest.hasPermission("ui"),
                      host.notifier,
                      QDir(directory).filePath("data").toUtf8(),
                      manifest.hasPermission("fs.instance"),
                      manifest.hasPermission("net"),
                      host.http,
                      (QDir(directory).filePath("?.lua") + ';' + QDir(directory).filePath("?/init.lua")).toUtf8(),
                      // Native modules are not supported: the launcher does not export the Lua API they link against
                      QByteArray(),
                      directory.toUtf8(),
                      // Each plugin is a folder inside the plugins folder
                      QFileInfo(directory).absolutePath().toUtf8(),
                      manifest.hasPermission("fs.plugins") };
    for (const QString& permission : manifest.permissions) {
        args.permissions.append(permission.toUtf8());
    }
    lua_State* L = state.get();
    lua_pushcfunction(L, openPrism);
    lua_pushlightuserdata(L, &args);
    if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
        const QString error = QString::fromUtf8(lua_tostring(L, -1));
        lua_pop(L, 1);
        return error;
    }
    return std::nullopt;
}

std::optional<QString> launchSteps(LuaState& state, QList<LaunchStepInfo>& steps)
{
    lua_State* L = state.get();
    lua_pushcfunction(L, collectLaunchSteps);
    lua_pushlightuserdata(L, &steps);
    return state.call(1);
}

std::optional<QString> runLaunchStep(LuaState& state, int index, LaunchContext& context)
{
    lua_State* L = state.get();
    RunStepArgs args{ index, &context };
    lua_pushcfunction(L, runStep);
    lua_pushlightuserdata(L, &args);
    auto error = state.call(1);

    // Neither of these raises errors
    if (lua_rawgetp(L, LUA_REGISTRYINDEX, &s_contextKey) == LUA_TUSERDATA) {
        *static_cast<LaunchContext**>(lua_touserdata(L, -1)) = nullptr;
    }
    lua_pop(L, 1);
    return error;
}

std::optional<QString> uiActions(LuaState& state, QList<UiActionInfo>& actions)
{
    lua_State* L = state.get();
    lua_pushcfunction(L, collectUiActions);
    lua_pushlightuserdata(L, &actions);
    return state.call(1);
}

std::optional<QString> runUiAction(LuaState& state, int index, const InstanceInfo* instance)
{
    lua_State* L = state.get();
    RunActionArgs args{ index, instance };
    lua_pushcfunction(L, runAction);
    lua_pushlightuserdata(L, &args);
    return state.call(1);
}

std::optional<QString> emitEvent(LuaState& state, const char* event, const EventFields& fields)
{
    lua_State* L = state.get();
    DispatchArgs args{ event, &fields };
    lua_pushcfunction(L, dispatch);
    lua_pushlightuserdata(L, &args);
    return state.call(1);
}

}  // namespace PrismApi
