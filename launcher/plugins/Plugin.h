// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QString>

#include <memory>

#include "Result.h"
#include "plugins/PluginManifest.h"
#include "plugins/lua/PrismApi.h"

class LuaState;

// A plugin folder (<dataRoot>/plugins/<id>). The manifest is read on construction so the plugin can be listed even
// when it is not running; load() runs its entry script in a fresh LuaState.
class Plugin {
   public:
    enum class State { Disabled, Loaded, Error };

    static constexpr qint64 s_maxEntrySize = 1024 * 1024;

    // host provides what the API offers (instances, notifications, network) and must outlive the plugin
    explicit Plugin(const QString& directory, const PrismApi::Host& host = {});
    ~Plugin();

    Plugin(const Plugin&) = delete;
    Plugin& operator=(const Plugin&) = delete;

    // The manifest id, or the folder name if the manifest is invalid
    QString id() const;
    QString directory() const { return m_directory; }
    const Result<PluginManifest>& manifest() const { return m_manifest; }

    State state() const { return m_state; }
    // Why the plugin is in the Error state, empty otherwise
    QString error() const { return m_error; }

    // Only valid while Loaded
    LuaState* lua() const { return m_lua.get(); }

    // Does nothing if already loaded. Returns whether the plugin is loaded.
    bool load();
    void unload();

    // Calls the plugin's handlers for the event if it is loaded. A failing handler is logged, the plugin stays loaded.
    void emitEvent(const char* event, const PrismApi::EventFields& fields = {});

    // Increases every time the plugin loads, so work prepared for one run can tell if it is still valid
    int loadCount() const { return m_loadCount; }

    // Puts the plugin in the Error state without loading it
    void fail(const QString& error);

   private:
    QString m_directory;
    Result<PluginManifest> m_manifest;
    State m_state = State::Disabled;
    QString m_error;
    PrismApi::Host m_host;
    std::unique_ptr<LuaState> m_lua;
    int m_loadCount = 0;
};
