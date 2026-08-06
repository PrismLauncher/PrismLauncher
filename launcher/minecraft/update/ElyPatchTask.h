// SPDX-License-Identifier: GPL-3.0-only
/*
 *  ElyPrismLauncher - Minecraft Launcher
 *  Copyright (c) 2025 Octol1ttle <l1ttleofficial@outlook.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <RuntimeContext.h>
#include <meta/Version.h>
#include <minecraft/LaunchProfile.h>

#include "tasks/Task.h"
struct RuntimeContext;
class MinecraftInstance;

class ElyPatchTask : public Task {
    Q_OBJECT
   public:
    ElyPatchTask(MinecraftInstance* inst, RuntimeContext& context, Net::Mode mode);
    virtual ~ElyPatchTask() = default;

    void executeTask() override;

    bool canAbort() const override;

   public slots:
    bool abort() override;

   private:
    void resolveAuthlib(QString version);
    void resolveAuthlibInjector();

    void applyMetaVersion(Meta::Version::Ptr metaVersion);
    void applyAuthlib(Meta::Version::Ptr metaVersion);

   private:
    MinecraftInstance* m_inst;
    RuntimeContext& m_runtimeContext;
    Net::Mode m_netMode;
    Task::Ptr m_currentTask;
};
