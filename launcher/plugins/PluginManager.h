// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QElapsedTimer>
#include <QHash>
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
class PluginManager : public QObject, public PrismApi::Notifier {
    Q_OBJECT

   public:
    // An action a loaded plugin registered with prism.ui.addAction
    struct Action {
        QString pluginId;
        QString pluginName;
        int index;
        QString text;
    };

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

    // Where the permissions the user granted to each plugin are kept, as a JSON object mapping ids to lists. Plugins
    // requesting permissions that were not granted are not loaded. Without it every permission counts as granted.
    void setGrantsSetting(std::shared_ptr<Setting> grants) { m_grants = std::move(grants); }
    // The permissions the plugin requests that the user has not granted
    QStringList missingPermissions(const QString& id) const;
    // Grants every permission the plugin requests now
    void grantPermissions(const QString& id);

    // Sends the event to every loaded plugin
    void emitEvent(const char* event, const PrismApi::EventFields& fields = {});
    // Sends launcher.ready to the loaded plugins, and from now on to each plugin right after it loads
    void setLauncherReady();

    // Inserts the steps the loaded plugins registered with prism.launch.addStep. Steps anchored to a phase that this
    // launch does not have are skipped.
    void contributeLaunchSteps(LaunchTask* task);
    static constexpr int s_maxGameLogLinesPerSecond = 200;

    // Sends launch.step, launch.finished and game.log for the task. game.log only carries the game's output (lines
    // logged during the launch phase, except the launcher's own), already censored, and drops lines beyond
    // s_maxGameLogLinesPerSecond.
    void watchLaunch(LaunchTask* task);
    // Both of the above, for a launch being created
    void prepareLaunch(LaunchTask* task);

    // Whether any loaded plugin has the ui permission, i.e. may add actions
    bool hasUiPlugins() const;
    // The actions of the loaded plugins for the scope, in plugin order
    QList<Action> actions(PrismApi::ActionScope scope) const;
    // Runs an action; instanceId selects the instance for instance actions. Returns the error message on failure.
    std::optional<QString> runAction(const Action& action, const QString& instanceId = {});

    static constexpr int s_maxNotificationsPerMinute = 5;

    // Logs the notification and emits notificationRequested, unless the plugin sent more than
    // s_maxNotificationsPerMinute in the last minute
    void notify(const QString& pluginId, const QString& text, PrismApi::NotifyLevel level) override;

    // Sends the requests made with prism.http; without it they fail. Takes effect for plugins loaded afterwards.
    void setHttpClient(std::unique_ptr<PrismApi::HttpClient> http) { m_http = std::move(http); }

    // Makes the list available to prism.instances and sends instance.created, instance.removed and instance.status for
    // its instances
    void watchInstances(InstanceList* instances);

   signals:
    // The list of plugins was rebuilt
    void pluginsChanged();
    // The state of one plugin changed
    void pluginChanged(const QString& id);
    // A plugin sent a notification with prism.ui.notify, within the rate limit
    void notificationRequested(const QString& pluginId, const QString& pluginName, const QString& text, PrismApi::NotifyLevel level);

   private:
    void watchInstance(BaseInstance* instance);
    QStringList enabledIds() const;
    bool loadPlugin(Plugin* plugin);

    QString m_pluginsDir;
    std::shared_ptr<Setting> m_enabledPlugins;
    std::shared_ptr<Setting> m_grants;
    // Declared before the plugins, which use them until they are destroyed
    std::unique_ptr<PrismApi::InstanceSource> m_instanceSource;
    std::unique_ptr<PrismApi::HttpClient> m_http;
    std::vector<std::unique_ptr<Plugin>> m_plugins;
    bool m_launcherReady = false;
    QElapsedTimer m_clock;
    // When each plugin's recent notifications were sent, in m_clock milliseconds
    QHash<QString, QList<qint64>> m_notificationTimes;
};
