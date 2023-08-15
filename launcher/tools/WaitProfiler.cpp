// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2023 Trial97 <alexandru.tripon97@gmail.com>
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
#include "WaitProfiler.h"

#include "BaseInstance.h"
#include "launch/LaunchTask.h"
#include "settings/SettingsObject.h"

class WaitProfiler : public BaseProfiler {
    Q_OBJECT
   public:
    WaitProfiler(SettingsObjectPtr settings, InstancePtr instance, QObject* parent = 0);

   protected:
    void beginProfilingImpl(shared_qobject_ptr<LaunchTask> process);
};

WaitProfiler::WaitProfiler(SettingsObjectPtr settings, InstancePtr instance, QObject* parent) : BaseProfiler(settings, instance, parent) {}

void WaitProfiler::beginProfilingImpl(shared_qobject_ptr<LaunchTask> process)
{
    emit readyToLaunch(tr("Started process: %1").arg(process->pid()));
}

BaseExternalTool* WaitProfilerFactory::createTool(InstancePtr instance, QObject* parent)
{
    return new WaitProfiler(globalSettings, instance, parent);
}
#include "WaitProfiler.moc"