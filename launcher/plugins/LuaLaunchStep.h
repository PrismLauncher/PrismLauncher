// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QPointer>

#include "launch/LaunchStep.h"

class PluginManager;

// Runs a step a plugin registered with prism.launch.addStep. A failing step is logged and the launch goes on; only
// ctx.abort() stops it. If the plugin was unloaded or reloaded since the launch was prepared, the step is skipped.
class LuaLaunchStep : public LaunchStep {
    Q_OBJECT

   public:
    LuaLaunchStep(LaunchTask* parent, PluginManager* manager, const QString& pluginId, int loadCount, int index, const QString& name);
    ~LuaLaunchStep() override = default;

   protected:
    void executeTask() override;

   private:
    QPointer<PluginManager> m_manager;
    QString m_pluginId;
    int m_loadCount;
    int m_index;
    QString m_name;
};
