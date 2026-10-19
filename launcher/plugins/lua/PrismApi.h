// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

#include <functional>
#include <optional>
#include <variant>

class LuaState;
struct PluginManifest;

namespace PrismApi {

// Longer log messages are truncated
constexpr qsizetype s_maxLogMessageLength = 4096;

// Creates the global `prism` table for the plugin described by the manifest, whose folder is directory, and routes
// `print` to prism.log.info. Returns the error message on failure.
std::optional<QString> install(LuaState& state, const PluginManifest& manifest, const QString& directory);

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
