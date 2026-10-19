// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "PluginManager.h"

#include <QDebug>
#include <QDir>

#include <algorithm>

#include "launch/LaunchTask.h"
#include "plugins/Logging.h"
#include "plugins/LuaLaunchStep.h"
#include "plugins/Plugin.h"
#include "plugins/lua/PrismApi.h"
#include "settings/Setting.h"

PluginManager::PluginManager(const QString& pluginsDir, std::shared_ptr<Setting> enabledPlugins, QObject* parent)
    : QObject(parent), m_pluginsDir(pluginsDir), m_enabledPlugins(std::move(enabledPlugins))
{}

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
        m_plugins.push_back(std::make_unique<Plugin>(dir.filePath(folder)));
    }

    const QStringList enabled = enabledIds();
    for (const auto& plugin : m_plugins) {
        if (enabled.contains(plugin->id())) {
            loadPlugin(plugin.get());
        }
    }
    emit pluginsChanged();
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

void PluginManager::emitEvent(const char* event)
{
    for (const auto& plugin : m_plugins) {
        plugin->emitEvent(event);
    }
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

bool PluginManager::loadPlugin(Plugin* plugin)
{
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
