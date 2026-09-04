// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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

#include "ScanModFolders.h"
#include "FileSystem.h"
#include "MMCZip.h"
#include "launch/LaunchTask.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/mod/ModFolderModel.h"
#include "minecraft/mod/ModProfileKeys.h"
#include "settings/SettingsObject.h"
#include "settings/Setting.h"
namespace {

bool applyLastActiveProfile(ModFolderModel* model,
                            SettingsObject* settings,
                            const QString& prefix,
                            QSet<QString>* outApplied = nullptr)
{
    auto profileListSetting = settings->getOrRegisterSetting(ModProfileKeys::profileListKey(prefix), QStringList());
    QStringList profileList = profileListSetting->get().toStringList();
    if (profileList.isEmpty()) {
        return false;
    }

    auto nameSetting = settings->getOrRegisterSetting(ModProfileKeys::lastActiveProfileNameKey(prefix), QString());
    QString profileName = nameSetting->get().toString();
    if (profileName.isEmpty() || !profileList.contains(profileName)) {
        auto indexSetting = settings->getOrRegisterSetting(ModProfileKeys::lastActiveIndexKey(prefix), 0);
        int lastActiveIndex = indexSetting->get().toInt();
        if (lastActiveIndex < 0 || lastActiveIndex >= profileList.size())
            lastActiveIndex = 0;
        profileName = profileList.at(lastActiveIndex);
    }

    QString profileKeyStr = ModProfileKeys::profileKey(prefix, profileName);
    auto profileSetting = settings->getOrRegisterSetting(profileKeyStr, QStringList());
    QStringList ids = profileSetting->get().toStringList();

    // An absent profile key means the filesystem remains authoritative.
    bool hasPersistedState = settings->containsValue(profileKeyStr);

    if (!hasPersistedState) {
        return false;
    }

    QSet<QString> enabledModIds(ids.begin(), ids.end());
    model->applyEnabledIds(enabledModIds);
    if (outApplied)
        *outApplied = enabledModIds;
    return true;
}

QSet<QString> applyForModel(ModFolderModel* model, SettingsObject* settings, const QString& prefix)
{
    QSet<QString> enabledModIds;
    bool haveExplicitConfig = false;

    auto defaultSelectedSetting = settings->getOrRegisterSetting(
        ModProfileKeys::overviewDefaultSelectedKey(prefix), true);
    bool useDefault = defaultSelectedSetting->get().toBool();

    if (useDefault) {
        auto profileListSetting = settings->getOrRegisterSetting(
            ModProfileKeys::profileListKey(prefix), QStringList());
        QStringList profileList = profileListSetting->get().toStringList();
        if (!profileList.isEmpty()) {
            QString defaultName = profileList.at(0);
            QString profileKeyStr = ModProfileKeys::profileKey(prefix, defaultName);
            auto profileSetting = settings->getOrRegisterSetting(profileKeyStr, QStringList());

            // An absent profile key means the filesystem remains authoritative.
            if (settings->containsValue(profileKeyStr)) {
                QStringList ids = profileSetting->get().toStringList();
                enabledModIds = QSet<QString>(ids.begin(), ids.end());
                haveExplicitConfig = true;
            }
        }
    } else {
        auto rtSetting = settings->getOrRegisterSetting(ModProfileKeys::runtimeProfilesKey(prefix), QStringList());
        QStringList selected = rtSetting->get().toStringList();
        if (selected.isEmpty()) {
            QSet<QString> fallback;
            applyLastActiveProfile(model, settings, prefix, &fallback);
            model->setLaunchSnapshot(fallback);
            return fallback;
        }
        for (const QString& profileName : std::as_const(selected)) {
            auto profileSetting = settings->getOrRegisterSetting(
                ModProfileKeys::profileKey(prefix, profileName), QStringList());
            const QStringList modIds = profileSetting->get().toStringList();
            for (const QString& id : modIds)
                enabledModIds.insert(id);
        }
        haveExplicitConfig = true;
    }

    if (haveExplicitConfig) {
        model->applyEnabledIds(enabledModIds);
    }
    model->setLaunchSnapshot(enabledModIds);
    return enabledModIds;
}

}  // namespace

void ScanModFolders::executeTask()
{
    auto m_inst = m_parent->instance();

    auto loaders = m_inst->loaderModList();
    connect(loaders, &ModFolderModel::updateFinished, this, &ScanModFolders::modsDone, Qt::SingleShotConnection);
    connect(loaders, &ModFolderModel::parseFinished, this, &ScanModFolders::onParseFinished);
    if (!loaders->update()) {
        m_modsDone = true;
    }

    auto cores = m_inst->coreModList();
    connect(cores, &ModFolderModel::updateFinished, this, &ScanModFolders::coreModsDone, Qt::SingleShotConnection);
    connect(cores, &ModFolderModel::parseFinished, this, &ScanModFolders::onParseFinished);
    if (!cores->update()) {
        m_coreModsDone = true;
    }

    auto nils = m_inst->nilModList();
    connect(nils, &ModFolderModel::updateFinished, this, &ScanModFolders::nilModsDone, Qt::SingleShotConnection);
    connect(nils, &ModFolderModel::parseFinished, this, &ScanModFolders::onParseFinished);
    if (!nils->update()) {
        m_nilModsDone = true;
    }
    checkDone();
}

void ScanModFolders::modsDone()
{
    m_modsDone = true;
    checkDone();
}

void ScanModFolders::coreModsDone()
{
    m_coreModsDone = true;
    checkDone();
}

void ScanModFolders::nilModsDone()
{
    m_nilModsDone = true;
    checkDone();
}

void ScanModFolders::onParseFinished()
{
    checkDone();
}
void ScanModFolders::checkDone()
{
    if (m_applyDone)
        return;
    if (!(m_modsDone && m_coreModsDone && m_nilModsDone))
        return;
    if (m_parent->instance()->loaderModList()->hasPendingParseTasks() ||
        m_parent->instance()->coreModList()->hasPendingParseTasks() ||
        m_parent->instance()->nilModList()->hasPendingParseTasks()) {
        return;
    }
    m_applyDone = true;
    applyRuntimeProfiles();
    emitSucceeded();
}
void ScanModFolders::applyRuntimeProfiles()
{
    auto* mcInst = dynamic_cast<MinecraftInstance*>(m_parent->instance());
    if (!mcInst)
        return;
    auto* settings = mcInst->settings();
    applyForModel(mcInst->loaderModList(), settings, QStringLiteral("mods"));
    applyForModel(mcInst->coreModList(),   settings, QStringLiteral("coremods"));
    applyForModel(mcInst->nilModList(),    settings, QStringLiteral("nilmods"));
    if (!m_restoreListenerConnected) {
        m_restoreListenerConnected = true;
        connect(mcInst, &BaseInstance::runningStatusChanged, this, [this, mcInst](bool running) {
            if (running)
                return;
            auto* settings = mcInst->settings();
            applyLastActiveProfile(mcInst->loaderModList(), settings, QStringLiteral("mods"));
            applyLastActiveProfile(mcInst->coreModList(),   settings, QStringLiteral("coremods"));
            applyLastActiveProfile(mcInst->nilModList(),    settings, QStringLiteral("nilmods"));
            mcInst->loaderModList()->clearLaunchSnapshot();
            mcInst->coreModList()->clearLaunchSnapshot();
            mcInst->nilModList()->clearLaunchSnapshot();
        });
    }
}
