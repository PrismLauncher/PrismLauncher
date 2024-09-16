// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#pragma once
#include "BaseInstance.h"
#include "launch/LaunchTask.h"

class NullInstance : public BaseInstance {
    Q_OBJECT
   public:
    NullInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString& rootDir)
        : BaseInstance(globalSettings, settings, rootDir)
    {
        setVersionBroken(true);
    }
    virtual ~NullInstance() = default;
    void saveNow() override {}
    void loadSpecificSettings() override { setSpecificSettingsLoaded(true); }
    QString getStatusbarDescription() override { return tr("Unknown instance type"); };
    QSet<QString> traits() const override { return {}; };
    QString instanceConfigFolder() const override { return instanceRoot(); };
    shared_qobject_ptr<LaunchTask> createLaunchTask(AuthSessionPtr, MinecraftTarget::Ptr) override { return nullptr; }
    QList<Task::Ptr> createUpdateTask() override { return {}; }
    QProcessEnvironment createEnvironment() override { return QProcessEnvironment(); }
    QProcessEnvironment createLaunchEnvironment() override { return QProcessEnvironment(); }
    QMap<QString, QString> getVariables() override { return QMap<QString, QString>(); }
    IPathMatcher::Ptr getLogFileMatcher() override { return nullptr; }
    QString getLogFileRoot() override { return instanceRoot(); }
    QString typeName() const override { return "Null"; }
    bool canExport() const override { return false; }
    bool canEdit() const override { return false; }
    bool canLaunch() const override { return false; }
    void populateLaunchMenu(QMenu* menu) override {}
    QStringList verboseDescription(AuthSessionPtr session, MinecraftTarget::Ptr targetToJoin) override
    {
        QStringList out;
        out << "Null instance - placeholder.";
        return out;
    }
    QString modsRoot() const override { return QString(); }
    void updateRuntimeContext()
    {
        // NOOP
    }
};
