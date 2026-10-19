// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "PluginManager.h"

#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFileSystemWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QTemporaryDir>
#include <QTimer>

#include <utility>

#include <algorithm>
#include <memory>

#include "BaseInstance.h"
#include "FileSystem.h"
#include "InstanceList.h"
#include "MMCZip.h"
#include "launch/LaunchStepIds.h"
#include "launch/LaunchTask.h"
#include "plugins/Logging.h"
#include "plugins/LuaLaunchStep.h"
#include "plugins/Plugin.h"
#include "plugins/lua/PrismApi.h"
#include "settings/Setting.h"

namespace {

class InstanceListSource : public PrismApi::InstanceSource {
   public:
    QList<PrismApi::InstanceInfo> instances() const override
    {
        QList<PrismApi::InstanceInfo> result;
        if (!list) {
            return result;
        }
        for (int i = 0; i < list->count(); i++) {
            const BaseInstance* instance = list->at(i);
            result.append({ instance->id().toUtf8(), instance->name().toUtf8(), list->getInstanceGroup(instance->id()).toUtf8(),
                            instance->isRunning(), instance->lastLaunch() / 1000, instance->totalTimePlayed() });
        }
        return result;
    }

    QString gameRoot(const QByteArray& id) const override
    {
        const BaseInstance* instance = list ? list->getInstanceById(QString::fromUtf8(id)) : nullptr;
        return instance ? instance->gameRoot() : QString();
    }

    QPointer<InstanceList> list;
};

}  // namespace

PluginManager::PluginManager(const QString& pluginsDir, std::shared_ptr<Setting> enabledPlugins, QObject* parent)
    : QObject(parent)
    , m_pluginsDir(pluginsDir)
    , m_enabledPlugins(std::move(enabledPlugins))
    , m_instanceSource(std::make_unique<InstanceListSource>())
{
    m_clock.start();
}

PluginManager::~PluginManager() = default;

void PluginManager::discover()
{
    m_plugins.clear();

    QDir dir(m_pluginsDir);
    if (!dir.mkpath(".")) {
        qWarning() << "Could not create the plugins folder" << m_pluginsDir;
    }
    // Hidden folders are skipped, so they can hold launcher data in the future.
    // A plugin's id is always its folder name, so this also sorts by id.
    const QStringList folders = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString& folder : folders) {
        m_plugins.push_back(std::make_unique<Plugin>(dir.filePath(folder), PrismApi::Host{ m_instanceSource.get(), this, m_http.get() }));
    }

    const QStringList enabled = enabledIds();
    for (const auto& plugin : m_plugins) {
        if (enabled.contains(plugin->id())) {
            loadPlugin(plugin.get());
        }
    }
    watchPlugins();
    emit pluginsChanged();
}

void PluginManager::reloadPlugin(const QString& id)
{
    Plugin* plugin = this->plugin(id);
    if (!plugin) {
        return;
    }
    plugin->refresh();
    if (isEnabled(id)) {
        loadPlugin(plugin);
    }
    qInfo() << "Reloaded plugin" << id;
    m_fingerprints[id] = codeFingerprint(plugin->directory());
    emit pluginChanged(id);
}

Result<QString> PluginManager::installFromZip(const QString& zipPath,
                                              const std::function<bool(const PluginManifest&, const Plugin&)>& replace)
{
    // Hidden, so it is never taken for a plugin, and in the same folder, so moving it in place is a rename
    QTemporaryDir staging(QDir(m_pluginsDir).filePath(".install-XXXXXX"));
    if (!staging.isValid()) {
        return std::unexpected(tr("Could not create a temporary folder in %1.").arg(m_pluginsDir));
    }
    // Rejects entries that would land outside the folder
    if (auto extracted = MMCZip::extractDir(zipPath, staging.path()); !extracted) {
        return std::unexpected(tr("Could not extract %1: %2").arg(zipPath, extracted.error()));
    }

    int files = 0;
    qint64 size = 0;
    QDirIterator it(staging.path(), QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QFileInfo info = it.nextFileInfo();
        if (info.isSymLink()) {
            return std::unexpected(tr("The zip contains a symbolic link (%1), which plugins cannot have.").arg(info.fileName()));
        }
        if (info.isFile()) {
            files++;
            size += info.size();
        }
    }
    if (files > s_maxZipFiles || size > s_maxZipSize) {
        return std::unexpected(
            tr("The zip is too large: plugins can have up to %1 files and %2 MiB.").arg(s_maxZipFiles).arg(s_maxZipSize / 1024 / 1024));
    }

    // The manifest is at the top, or inside the only folder (zipping a folder does that); macOS adds __MACOSX
    QString root = staging.path();
    if (!QFile::exists(QDir(root).filePath("plugin.toml"))) {
        QStringList folders = QDir(root).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        folders.removeAll("__MACOSX");
        if (folders.size() != 1 || !QFile::exists(QDir(root).filePath(folders.first() + "/plugin.toml"))) {
            return std::unexpected(tr("The zip has no plugin.toml at its top or inside its only folder."));
        }
        root = QDir(root).filePath(folders.first());
    }
    auto manifest = PluginManifest::load(QDir(root).filePath("plugin.toml"));
    if (!manifest) {
        return std::unexpected(tr("The plugin.toml in the zip is not valid: %1").arg(manifest.error()));
    }

    const QString id = manifest->id;
    const QString target = QDir(m_pluginsDir).filePath(id);
    Plugin* existing = plugin(id);
    if (existing || QFileInfo::exists(target)) {
        if (!existing || !replace(*manifest, *existing)) {
            return std::unexpected(tr("A plugin with the id %1 is already installed.").arg(id));
        }
        existing->unload();
        // The user's settings and the plugin's data survive the update
        for (const QString& kept : { QString("settings.ini"), QString("data") }) {
            const QString from = QDir(target).filePath(kept);
            const QString to = QDir(root).filePath(kept);
            if (QFileInfo::exists(from) && !QFileInfo::exists(to) && !FS::move(from, to)) {
                return std::unexpected(tr("Could not keep %1 of the installed plugin.").arg(kept));
            }
        }
        if (!FS::deletePath(target)) {
            return std::unexpected(tr("Could not remove the installed version of %1.").arg(id));
        }
    }
    if (!QDir().rename(root, target)) {
        return std::unexpected(tr("Could not move the plugin into %1.").arg(target));
    }

    if (existing) {
        reloadPlugin(id);
    } else {
        m_plugins.push_back(std::make_unique<Plugin>(target, PrismApi::Host{ m_instanceSource.get(), this, m_http.get() }));
        std::sort(m_plugins.begin(), m_plugins.end(), [](const auto& a, const auto& b) { return a->id() < b->id(); });
        watchPlugins();
        emit pluginsChanged();
    }
    qInfo() << "Installed plugin" << id << "from" << zipPath;
    return id;
}

void PluginManager::setAutoReload(bool enabled)
{
    if (!enabled) {
        m_watcher.reset();
        return;
    }
    if (m_watcher) {
        return;
    }
    m_watcher = std::make_unique<QFileSystemWatcher>();
    connect(m_watcher.get(), &QFileSystemWatcher::fileChanged, this, &PluginManager::onPathChanged);
    connect(m_watcher.get(), &QFileSystemWatcher::directoryChanged, this, &PluginManager::onPathChanged);
    if (!m_reloadTimer) {
        m_reloadTimer = new QTimer(this);
        m_reloadTimer->setSingleShot(true);
        m_reloadTimer->setInterval(s_reloadDelayMs);
        connect(m_reloadTimer, &QTimer::timeout, this, &PluginManager::reloadChangedPlugins);
    }
    watchPlugins();
}

QStringList PluginManager::codeFingerprint(const QString& directory)
{
    QStringList fingerprint;
    QDirIterator it(directory, { "*.lua", "plugin.toml" }, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QFileInfo info = it.nextFileInfo();
        const QString relative = QDir(directory).relativeFilePath(info.filePath());
        // The plugin's own data is not code
        if (relative.startsWith("data/") || relative.startsWith('.')) {
            continue;
        }
        fingerprint.append(QString("%1|%2|%3").arg(relative).arg(info.size()).arg(info.lastModified().toMSecsSinceEpoch()));
    }
    fingerprint.sort();
    return fingerprint;
}

void PluginManager::watchPlugins()
{
    if (!m_watcher) {
        return;
    }
    const QStringList watched = m_watcher->files() + m_watcher->directories();
    if (!watched.isEmpty()) {
        m_watcher->removePaths(watched);
    }
    m_fingerprints.clear();
    QStringList paths;
    for (const auto& plugin : m_plugins) {
        const QString directory = plugin->directory();
        m_fingerprints[plugin->id()] = codeFingerprint(directory);
        // Folders tell about added and removed files, files about changes
        paths.append(directory);
        QDirIterator it(directory, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString folder = it.next();
            if (!QDir(directory).relativeFilePath(folder).startsWith("data")) {
                paths.append(folder);
            }
        }
        QDirIterator files(directory, { "*.lua", "plugin.toml" }, QDir::Files, QDirIterator::Subdirectories);
        while (files.hasNext()) {
            paths.append(files.next());
        }
    }
    if (!paths.isEmpty()) {
        m_watcher->addPaths(paths);
    }
}

void PluginManager::onPathChanged(const QString& path)
{
    for (const auto& plugin : m_plugins) {
        const QString directory = plugin->directory();
        if (path == directory || path.startsWith(directory + '/')) {
            m_pendingReloads.insert(plugin->id());
        }
    }
    // Waits for the burst of changes a save causes to end
    m_reloadTimer->start();
}

void PluginManager::reloadChangedPlugins()
{
    const QSet<QString> pending = std::exchange(m_pendingReloads, {});
    for (const QString& id : pending) {
        const Plugin* plugin = this->plugin(id);
        if (plugin && codeFingerprint(plugin->directory()) != m_fingerprints.value(id)) {
            qInfo() << "Plugin" << id << "changed on disk";
            reloadPlugin(id);
        }
    }
    // Saving often replaces files, which drops them from the watcher
    watchPlugins();
}

QList<Plugin*> PluginManager::plugins() const
{
    QList<Plugin*> plugins;
    plugins.reserve(m_plugins.size());
    for (const auto& plugin : m_plugins) {
        plugins.append(plugin.get());
    }
    return plugins;
}

Plugin* PluginManager::plugin(const QString& id) const
{
    auto it = std::find_if(m_plugins.begin(), m_plugins.end(), [&id](const auto& plugin) { return plugin->id() == id; });
    return it != m_plugins.end() ? it->get() : nullptr;
}

bool PluginManager::isEnabled(const QString& id) const
{
    return enabledIds().contains(id);
}

bool PluginManager::setEnabled(const QString& id, bool enabled)
{
    Plugin* plugin = this->plugin(id);
    if (!plugin) {
        return false;
    }

    // Ids of plugins that are not installed right now are kept, so removing a folder temporarily keeps its choice
    QStringList ids = enabledIds();
    ids.removeAll(id);
    if (enabled) {
        ids.append(id);
        ids.sort();
    }
    m_enabledPlugins->set(ids);

    bool ok = true;
    if (enabled) {
        ok = loadPlugin(plugin);
    } else {
        plugin->unload();
    }
    emit pluginChanged(id);
    return ok;
}

void PluginManager::emitEvent(const char* event, const PrismApi::EventFields& fields)
{
    for (const auto& plugin : m_plugins) {
        plugin->emitEvent(event, fields);
    }
}

void PluginManager::watchLaunch(LaunchTask* task)
{
    struct GameLog {
        // Only the lines logged during the launch phase come from the game
        bool gameRunning = false;
        QElapsedTimer window;
        int lines = 0;
        qint64 skipped = 0;
    };
    auto gameLog = std::make_shared<GameLog>();

    const QByteArray instanceId = task->instance() ? task->instance()->id().toUtf8() : QByteArray();
    connect(task, &LaunchTask::stepStarted, this, [this, instanceId, gameLog](const QString& step) {
        gameLog->gameRunning = step == LaunchStepIds::Launch;
        emitEvent("launch.step", { { "id", instanceId }, { "step", step.toUtf8() } });
    });
    connect(task, &Task::finished, this, [this, task, instanceId, gameLog] {
        gameLog->gameRunning = false;
        emitEvent("launch.finished",
                  { { "id", instanceId }, { "success", task->wasSuccessful() }, { "reason", task->failReason().toUtf8() } });
    });

    connect(task, &LaunchTask::logLineAdded, this, [this, instanceId, gameLog](const QString& line, MessageLevel level) {
        if (!gameLog->gameRunning || level == MessageLevel::Launcher) {
            return;
        }
        if (!gameLog->window.isValid() || gameLog->window.elapsed() >= 1000) {
            gameLog->window.start();
            gameLog->lines = 0;
        }
        if (++gameLog->lines > s_maxGameLogLinesPerSecond) {
            gameLog->skipped++;
            return;
        }
        emitEvent("game.log", { { "id", instanceId },
                                { "line", line.toUtf8() },
                                { "level", level.toString().toLower().toUtf8() },
                                { "skipped", gameLog->skipped } });
        gameLog->skipped = 0;
    });
}

void PluginManager::prepareLaunch(LaunchTask* task)
{
    contributeLaunchSteps(task);
    watchLaunch(task);
}

void PluginManager::notify(const QString& pluginId, const QString& text, PrismApi::NotifyLevel level)
{
    auto& times = m_notificationTimes[pluginId];
    const qint64 now = m_clock.elapsed();
    times.removeIf([now](qint64 time) { return now - time >= 60'000; });
    if (times.size() >= s_maxNotificationsPerMinute) {
        qCWarning(pluginLogC).noquote() << QString("[%1] notification dropped, too many in the last minute: %2").arg(pluginId, text);
        return;
    }
    times.append(now);

    qCInfo(pluginLogC).noquote() << QString("[%1] notification: %2").arg(pluginId, text);
    const Plugin* plugin = this->plugin(pluginId);
    const QString name = plugin && plugin->manifest() ? plugin->manifest()->name : pluginId;
    emit notificationRequested(pluginId, name, text, level);
}

bool PluginManager::hasUiPlugins() const
{
    return std::any_of(m_plugins.begin(), m_plugins.end(), [](const auto& plugin) {
        return plugin->state() == Plugin::State::Loaded && plugin->manifest()->hasPermission("ui");
    });
}

QList<PluginManager::Action> PluginManager::actions(PrismApi::ActionScope scope) const
{
    QList<Action> result;
    for (const auto& plugin : m_plugins) {
        if (plugin->state() != Plugin::State::Loaded || !plugin->manifest()->hasPermission("ui")) {
            continue;
        }
        QList<PrismApi::UiActionInfo> actions;
        if (auto error = PrismApi::uiActions(*plugin->lua(), actions)) {
            qCWarning(pluginLogC).noquote() << QString("[%1] could not read actions: %2").arg(plugin->id(), *error);
            continue;
        }
        for (const auto& action : actions) {
            if (action.scope == scope) {
                result.append({ plugin->id(), plugin->manifest()->name, action.index, action.text });
            }
        }
    }
    return result;
}

std::optional<QString> PluginManager::runAction(const Action& action, const QString& instanceId)
{
    Plugin* plugin = this->plugin(action.pluginId);
    if (!plugin || plugin->state() != Plugin::State::Loaded) {
        return tr("The plugin %1 is not running.").arg(action.pluginId);
    }

    std::optional<PrismApi::InstanceInfo> instance;
    if (!instanceId.isEmpty()) {
        const QByteArray id = instanceId.toUtf8();
        for (const auto& info : m_instanceSource->instances()) {
            if (info.id == id) {
                instance = info;
            }
        }
        if (!instance) {
            return tr("The instance %1 does not exist.").arg(instanceId);
        }
    }

    auto error = PrismApi::runUiAction(*plugin->lua(), action.index, instance ? &*instance : nullptr);
    if (error) {
        qCWarning(pluginLogC).noquote() << QString("[%1] action \"%2\" failed: %3").arg(plugin->id(), action.text, *error);
    }
    return error;
}

void PluginManager::watchInstances(InstanceList* instances)
{
    static_cast<InstanceListSource*>(m_instanceSource.get())->list = instances;
    for (int i = 0; i < instances->count(); i++) {
        watchInstance(instances->at(i));
    }
    connect(instances, &QAbstractItemModel::rowsInserted, this, [this, instances](const QModelIndex&, int first, int last) {
        for (int i = first; i <= last; i++) {
            BaseInstance* instance = instances->at(i);
            watchInstance(instance);
            emitEvent("instance.created", { { "id", instance->id().toUtf8() }, { "name", instance->name().toUtf8() } });
        }
    });
    connect(instances, &QAbstractItemModel::rowsAboutToBeRemoved, this, [this, instances](const QModelIndex&, int first, int last) {
        for (int i = first; i <= last; i++) {
            BaseInstance* instance = instances->at(i);
            emitEvent("instance.removed", { { "id", instance->id().toUtf8() }, { "name", instance->name().toUtf8() } });
        }
    });
}

void PluginManager::watchInstance(BaseInstance* instance)
{
    connect(instance, &BaseInstance::runningStatusChanged, this, [this, instance](bool running) {
        emitEvent("instance.status", { { "id", instance->id().toUtf8() }, { "running", running } });
    });
}

void PluginManager::setLauncherReady()
{
    m_launcherReady = true;
    emitEvent("launcher.ready");
}

void PluginManager::contributeLaunchSteps(LaunchTask* task)
{
    struct Contribution {
        Plugin* plugin;
        PrismApi::LaunchStepInfo info;
    };
    QList<Contribution> contributions;
    for (const auto& plugin : m_plugins) {
        if (plugin->state() != Plugin::State::Loaded) {
            continue;
        }
        QList<PrismApi::LaunchStepInfo> steps;
        if (auto error = PrismApi::launchSteps(*plugin->lua(), steps)) {
            qCWarning(pluginLogC).noquote() << QString("[%1] could not read launch steps: %2").arg(plugin->id(), *error);
            continue;
        }
        for (const auto& step : steps) {
            contributions.append({ plugin.get(), step });
        }
    }

    // Each insertion goes right before the first or after the last step of its phase. To keep the registration order,
    // "before" steps are inserted in order and "after" steps in reverse.
    auto insert = [this, task](const Contribution& contribution) {
        Plugin* plugin = contribution.plugin;
        const auto& info = contribution.info;
        const QString name = info.name.isEmpty() ? plugin->manifest()->name : info.name;
        auto step = makeShared<LuaLaunchStep>(task, this, plugin->id(), plugin->loadCount(), info.index, name);
        step->setId("plugin:" + plugin->id());
        if (!task->insertStep(step, info.anchor, info.before ? LaunchTask::Position::Before : LaunchTask::Position::After)) {
            qCInfo(pluginLogC).noquote()
                << QString("[%1] skipping launch step \"%2\": this launch has no '%3' phase").arg(plugin->id(), name, info.anchor);
        }
    };
    for (const auto& contribution : contributions) {
        if (contribution.info.before) {
            insert(contribution);
        }
    }
    for (auto it = contributions.crbegin(); it != contributions.crend(); ++it) {
        if (!it->info.before) {
            insert(*it);
        }
    }
}

QStringList PluginManager::enabledIds() const
{
    return m_enabledPlugins->get().toStringList();
}

QStringList PluginManager::missingPermissions(const QString& id) const
{
    const Plugin* plugin = this->plugin(id);
    if (!m_grants || !plugin || !plugin->manifest()) {
        return {};
    }
    const QJsonObject grants = QJsonDocument::fromJson(m_grants->get().toString().toUtf8()).object();
    const QStringList granted = grants.value(id).toVariant().toStringList();
    QStringList missing;
    for (const QString& permission : plugin->manifest()->permissions) {
        if (!granted.contains(permission)) {
            missing.append(permission);
        }
    }
    return missing;
}

void PluginManager::grantPermissions(const QString& id)
{
    const Plugin* plugin = this->plugin(id);
    if (!m_grants || !plugin || !plugin->manifest()) {
        return;
    }
    QJsonObject grants = QJsonDocument::fromJson(m_grants->get().toString().toUtf8()).object();
    grants.insert(id, QJsonArray::fromStringList(plugin->manifest()->permissions));
    m_grants->set(QString::fromUtf8(QJsonDocument(grants).toJson(QJsonDocument::Compact)));
}

bool PluginManager::loadPlugin(Plugin* plugin)
{
    if (const QStringList missing = missingPermissions(plugin->id()); !missing.isEmpty()) {
        plugin->fail(tr("Requests permissions you have not granted: %1. Enable it again to review them.").arg(missing.join(", ")));
        qWarning() << "Not loading plugin" << plugin->id() << "until its permissions are granted:" << missing;
        return false;
    }
    if (!plugin->load()) {
        qWarning() << "Failed to load plugin" << plugin->id() << ":" << plugin->error();
        return false;
    }
    qInfo() << "Loaded plugin" << plugin->id();
    if (m_launcherReady) {
        plugin->emitEvent("launcher.ready");
    }
    return true;
}
