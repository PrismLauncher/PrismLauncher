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
#include <QDebug>
#include "launch/LaunchTask.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/mod/ModFolderModel.h"
#include "minecraft/mod/ModProfileKeys.h"
#include "settings/SettingsObject.h"
#include "settings/Setting.h"

namespace {

// Applies the last-active EDITING profile's saved mod set directly (not a
// runtime union) via applyEnabledIds(). Shared by applyForModel()'s
// fallback (no runtime override configured) and by the post-launch restore
// below -- one function that knows how to compute "what the editing
// profile actually has saved," one primitive (applyEnabledIds) that writes
// it. Returns false and does nothing if this page type has never been
// configured at all (no ModProfileList_<prefix> entries).
bool applyLastActiveProfile(ModFolderModel* model, SettingsObject* settings, const QString& prefix)
{
    auto profileListSetting = settings->getOrRegisterSetting(ModProfileKeys::profileListKey(prefix), QStringList());
    QStringList profileList = profileListSetting->get().toStringList();
    if (profileList.isEmpty()) {
        qDebug() << "[ScanModFolders] applyLastActiveProfile no profile data ever configured,"
                    " leaving filesystem as-is" << "prefix=" << prefix;
        return false;
    }

    auto lastActiveSetting = settings->getOrRegisterSetting(ModProfileKeys::lastActiveIndexKey(prefix), 0);
    int lastActiveIndex = lastActiveSetting->get().toInt();
    if (lastActiveIndex < 0 || lastActiveIndex >= profileList.size()) {
        lastActiveIndex = 0;
    }
    QString profileName = profileList.at(lastActiveIndex);

    auto profileSetting = settings->getOrRegisterSetting(ModProfileKeys::profileKey(prefix, profileName), QStringList());
    const QStringList ids = profileSetting->get().toStringList();
    QSet<QString> enabledModIds(ids.begin(), ids.end());
    model->applyEnabledIds(enabledModIds);
    return true;
}

bool applyForModel(ModFolderModel* model, SettingsObject* settings, const QString& prefix)
{
    auto rtSetting = settings->getOrRegisterSetting(ModProfileKeys::runtimeProfilesKey(prefix), QStringList());
    QStringList selected = rtSetting->get().toStringList();
    qDebug() << "[ScanModFolders] applyForModel" << "prefix=" << prefix
             << "runtimeSelection=" << selected;

    if (selected.isEmpty()) {
        // No explicit runtime override -- apply the last-active editing
        // profile directly instead of assuming the filesystem already
        // matches it (a prior launch may have left a runtime union or a
        // different profile's state on disk with nothing to restore it --
        // see the post-launch restore trigger registered below).
        return applyLastActiveProfile(model, settings, prefix);
    }

    // Compute the union of enabled mod IDs across all selected profiles.
    QSet<QString> enabledModIds;
    for (const QString& profileName : std::as_const(selected)) {
        auto profileSetting = settings->getOrRegisterSetting(ModProfileKeys::profileKey(prefix, profileName), QStringList());
        const QStringList modIds = profileSetting->get().toStringList();
        for (const QString& id : modIds)
            enabledModIds.insert(id);
    }

    model->applyEnabledIds(enabledModIds);
    return true;
}

}  // namespace

void ScanModFolders::executeTask()
{
    auto m_inst = m_parent->instance();

    auto loaders = m_inst->loaderModList();
    connect(loaders, &ModFolderModel::updateFinished, this, &ScanModFolders::modsDone, Qt::SingleShotConnection);
    // The folder scans (updateFinished) complete long before the per-mod
    // parse tasks do -- applyEnabledIds() must not run on filename-fallback
    // identities (Investigation 09, Race A/B). checkDone() now also waits on
    // hasPendingParseTasks(); this signal re-runs it when the last parse
    // finishes. This connection must be PERSISTENT: parseFinished fires once
    // per completed parse task, so a single-shot connection would disconnect
    // after the first one and hang the launch forever if more were pending.
    // checkDone()'s m_applyDone guard makes the repeated calls harmless, and
    // the connection auto-cleans when this step is destroyed.
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
    // One of the folder models finished a per-mod parse task; re-check
    // whether every folder scan is done and no parse tasks remain pending.
    checkDone();
}

void ScanModFolders::checkDone()
{
    if (m_applyDone)
        return;

    if (!(m_modsDone && m_coreModsDone && m_nilModsDone))
        return;

    // Race A (Investigation 09): updateFinished fires when the folder scan
    // completes, but per-mod parse tasks may still be running. Applying
    // runtime profiles then matches saved ids against filename-fallback
    // identities, so a mod like cloth-config (slug "cloth-config" vs fallback
    // "Cloth Config v20") never matches and is left disabled. Race B: renaming
    // .jar/.jar.disabled while a LocalModParseTask is still reading the old
    // path produces "Failed to open archive file" and permanently loses the
    // parsed details. Both are removed by waiting for hasPendingParseTasks()
    // to become false -- the same guard the UI save path already trusts
    // (ResourceFolderModel.h:135, Investigation 09 §9.2). The parseFinished
    // connections from executeTask() re-run this check when that happens.
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

    // The filesystem now reflects a runtime override or the last-active
    // editing profile -- either way it must go back to matching the editing
    // profile once the game exits. ModFolderPage::restoreCurrentProfileToFilesystem()
    // already does this, but only if a page happens to exist (Edit Instance
    // opened this session) -- a launch from the main window never
    // constructs one, so nothing restores it (root-caused via full
    // architecture investigation: the mutation is launch-owned, the only
    // restoration trigger was UI-page-owned, and that page doesn't exist on
    // the default launch path).
    //
    // Fix: register our own restore trigger here, independent of any UI
    // page, reusing the exact same settings-derived-profile +
    // applyEnabledIds() primitive as applyForModel()'s own fallback above --
    // not a new/competing filesystem writer. Connect exactly once per
    // ScanModFolders lifetime (it persists across launches on this
    // instance, same reasoning as the Task.cpp:151 fix -- connecting fresh
    // every launch without disconnecting would accumulate duplicate
    // handlers).
    if (!m_restoreListenerConnected) {
        m_restoreListenerConnected = true;
        connect(mcInst, &BaseInstance::runningStatusChanged, this, [this, mcInst](bool running) {
            if (running)
                return;
            auto* settings = mcInst->settings();
            qDebug() << "[ScanModFolders] game exited, restoring editing profile to filesystem";
            applyLastActiveProfile(mcInst->loaderModList(), settings, QStringLiteral("mods"));
            applyLastActiveProfile(mcInst->coreModList(),   settings, QStringLiteral("coremods"));
            applyLastActiveProfile(mcInst->nilModList(),    settings, QStringLiteral("nilmods"));
        });
    }
}
