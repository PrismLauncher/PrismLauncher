// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

#include <memory>
#include <vector>

#include "plugins/lua/PrismApi.h"

class BaseInstance;
class InstanceList;
class LaunchTask;
class Plugin;
class Setting;

// Discovers the plugin folders in a directory and loads the enabled ones. Which plugins are enabled is stored as a
// list of ids in a setting; new plugins start disabled, so dropping a folder in is never enough to run its code.
class PluginManager : public QObject {
    Q_OBJECT

   public:
    PluginManager(const QString& pluginsDir, std::shared_ptr<Setting> enabledPlugins, QObject* parent = nullptr);
    ~PluginManager() override;

    QString pluginsDir() const { return m_pluginsDir; }

    // Unloads every plugin, rescans the plugins folder (creating it if needed) and loads the enabled plugins
    void discover();

    // Sorted by id
    QList<Plugin*> plugins() const;
    Plugin* plugin(const QString& id) const;

    bool isEnabled(const QString& id) const;
    // Remembers the choice and loads or unloads the plugin. Returns false if the plugin is unknown or failed to load.
    bool setEnabled(const QString& id, bool enabled);

    // Sends the event to every loaded plugin
    void emitEvent(const char* event, const PrismApi::EventFields& fields = {});
    // Sends launcher.ready to the loaded plugins, and from now on to each plugin right after it loads
    void setLauncherReady();

    // Inserts the steps the loaded plugins registered with prism.launch.addStep. Steps anchored to a phase that this
    // launch does not have are skipped.
    void contributeLaunchSteps(LaunchTask* task);
    static constexpr int s_maxGameLogLinesPerSecond = 200;

    // Sends launch.step, launch.finished and game.log for the task. game.log only carries the game's output (not the
    // launcher's own lines), already censored, and drops lines beyond s_maxGameLogLinesPerSecond.
    void watchLaunch(LaunchTask* task);
    // Both of the above, for a launch being created
    void prepareLaunch(LaunchTask* task);

    // Makes the list available to prism.instances and sends instance.created, instance.removed and instance.status for
    // its instances
    void watchInstances(InstanceList* instances);

   signals:
    // The list of plugins was rebuilt
    void pluginsChanged();
    // The state of one plugin changed
    void pluginChanged(const QString& id);

   private:
    void watchInstance(BaseInstance* instance);
    QStringList enabledIds() const;
    bool loadPlugin(Plugin* plugin);

    QString m_pluginsDir;
    std::shared_ptr<Setting> m_enabledPlugins;
    // Declared before the plugins, which use it until they are destroyed
    std::unique_ptr<PrismApi::InstanceSource> m_instanceSource;
    std::vector<std::unique_ptr<Plugin>> m_plugins;
    bool m_launcherReady = false;
};
