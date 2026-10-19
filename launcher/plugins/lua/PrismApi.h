// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QByteArray>
#include <QList>
#include <QPair>
#include <QString>
#include <QUrl>

#include <functional>
#include <optional>
#include <variant>

class LuaState;
struct PluginManifest;

namespace PrismApi {

// Longer log messages are truncated
constexpr qsizetype s_maxLogMessageLength = 4096;

// What plugins can see of an instance
struct InstanceInfo {
    QByteArray id;
    QByteArray name;
    QByteArray group;
    bool running = false;
    // Seconds since the epoch, 0 if never launched
    qint64 lastLaunch = 0;
    // Seconds
    qint64 playTime = 0;
};

// Where prism.instances gets its data from
class InstanceSource {
   public:
    virtual ~InstanceSource() = default;
    virtual QList<InstanceInfo> instances() const = 0;
    // The game folder (.minecraft) of the instance, empty if there is no such instance
    virtual QString gameRoot(const QByteArray& id) const = 0;

    // Holds the results while they are pushed to Lua, which may raise errors that skip C++ destructors
    QList<InstanceInfo> scratch;
};

enum class NotifyLevel { Info, Warning, Error };

// Receives the notifications sent with prism.ui.notify
class Notifier {
   public:
    virtual ~Notifier() = default;
    virtual void notify(const QString& pluginId, const QString& text, NotifyLevel level) = 0;
};

// Longer notifications are truncated
constexpr qsizetype s_maxNotificationLength = 300;

struct HttpRequest {
    QByteArray method;
    QUrl url;
    QList<QPair<QByteArray, QByteArray>> headers;
    QByteArray body;
    int timeoutMs = 30'000;
};

struct HttpResponse {
    // 0 when no response arrived
    int status = 0;
    QByteArray body;
    QList<QPair<QByteArray, QByteArray>> headers;
    // Why the request failed, empty if a response arrived (whatever its status)
    QString error;
};

// Sends the requests made with prism.http
class HttpClient {
   public:
    virtual ~HttpClient() = default;
    // Calls done exactly once, later, from the event loop
    virtual void send(const HttpRequest& request, std::function<void(const HttpResponse&)> done) = 0;
};

constexpr qint64 s_maxHttpResponseSize = 16 * 1024 * 1024;
constexpr int s_maxPendingHttpRequests = 8;
constexpr int s_maxHttpTimeoutSeconds = 120;

// What the launcher offers to the API. Everything must outlive the plugin's state; whatever is missing behaves as if
// there was nothing: no instances, notifications only logged, requests that fail.
struct Host {
    InstanceSource* instances = nullptr;
    Notifier* notifier = nullptr;
    HttpClient* http = nullptr;
};

// Creates the global `prism` table for the plugin described by the manifest, whose folder is directory, and routes
// `print` to prism.log.info. Returns the error message on failure.
std::optional<QString> install(LuaState& state, const PluginManifest& manifest, const QString& directory, const Host& host = {});

// A step registered with prism.launch.addStep
struct LaunchStepInfo {
    int index;
    QString anchor;
    bool before;
    // Empty if the plugin did not name it
    QString name;
};

constexpr int s_maxLaunchSteps = 16;

// Reads the steps registered with prism.launch.addStep. Returns the error message on failure.
std::optional<QString> launchSteps(LuaState& state, QList<LaunchStepInfo>& steps);

// What a launch step can see and do while it runs
struct LaunchContext {
    QByteArray instanceId;
    QByteArray instanceName;
    std::function<void(const QString&)> log;
    bool aborted = false;
    QString abortReason;
};

// Runs the step registered with the given index, passing it a ctx table backed by context. Returns the error message
// if the step failed; context.aborted tells whether it asked to abort the launch.
std::optional<QString> runLaunchStep(LuaState& state, int index, LaunchContext& context);

enum class ActionScope { Instance, Launcher };

// An action registered with prism.ui.addAction
struct UiActionInfo {
    int index;
    ActionScope scope;
    QString text;
};

constexpr int s_maxUiActions = 16;
constexpr qsizetype s_maxUiActionTextLength = 100;

// Reads the actions registered with prism.ui.addAction. Returns the error message on failure.
std::optional<QString> uiActions(LuaState& state, QList<UiActionInfo>& actions);

// Runs the action registered with the given index. ctx.instance holds the instance for instance actions, and is nil
// when instance is null. Returns the error message on failure.
std::optional<QString> runUiAction(LuaState& state, int index, const InstanceInfo* instance);

// A field of the table passed to event handlers
struct EventField {
    const char* key;
    std::variant<QByteArray, bool, qint64> value;
};
using EventFields = QList<EventField>;

// Calls the handlers registered with prism.on for the event, in registration order, passing each a table with the
// fields. Handlers limited to an instance only run if the "id" field matches. The first failing handler stops the
// others. Returns its error message.
std::optional<QString> emitEvent(LuaState& state, const char* event, const EventFields& fields = {});

}  // namespace PrismApi
