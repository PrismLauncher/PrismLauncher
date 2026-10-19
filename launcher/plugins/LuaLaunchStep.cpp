// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "LuaLaunchStep.h"

#include "launch/LaunchTask.h"
#include "plugins/Plugin.h"
#include "plugins/PluginManager.h"
#include "plugins/lua/PrismApi.h"

LuaLaunchStep::LuaLaunchStep(LaunchTask* parent,
                             PluginManager* manager,
                             const QString& pluginId,
                             int loadCount,
                             int index,
                             const QString& name)
    : LaunchStep(parent), m_manager(manager), m_pluginId(pluginId), m_loadCount(loadCount), m_index(index), m_name(name)
{}

void LuaLaunchStep::executeTask()
{
    Plugin* plugin = m_manager ? m_manager->plugin(m_pluginId) : nullptr;
    if (!plugin || plugin->state() != Plugin::State::Loaded || plugin->loadCount() != m_loadCount) {
        emit logLine(tr("Skipping \"%1\": plugin %2 was unloaded or reloaded.").arg(m_name, m_pluginId), MessageLevel::Warning);
        emitSucceeded();
        return;
    }

    emit logLine(tr("Running \"%1\" from plugin %2").arg(m_name, m_pluginId), MessageLevel::Launcher);

    PrismApi::LaunchContext context;
    if (MinecraftInstance* instance = m_parent->instance()) {
        context.instanceId = instance->id().toUtf8();
        context.instanceName = instance->name().toUtf8();
    }
    context.log = [this](const QString& line) { emit logLine(line, MessageLevel::Launcher); };

    auto error = PrismApi::runLaunchStep(*plugin->lua(), m_index, context);
    if (context.aborted) {
        emitFailed(tr("Plugin %1 aborted the launch: %2").arg(m_pluginId, context.abortReason));
        return;
    }
    if (error) {
        emit logLine(tr("\"%1\" from plugin %2 failed, continuing: %3").arg(m_name, m_pluginId, *error), MessageLevel::Warning);
    }
    emitSucceeded();
}
