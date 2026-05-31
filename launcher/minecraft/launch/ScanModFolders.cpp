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

void ScanModFolders::executeTask()
{
    auto m_inst = m_parent->instance();

    auto loaders = m_inst->loaderModList();
    connect(loaders, &ModFolderModel::updateFinished, this, &ScanModFolders::modsDone, Qt::UniqueConnection);
    if (!loaders->update()) {
        m_modsDone = true;
    }

    auto cores = m_inst->coreModList();
    connect(cores, &ModFolderModel::updateFinished, this, &ScanModFolders::coreModsDone, Qt::UniqueConnection);
    if (!cores->update()) {
        m_coreModsDone = true;
    }

    auto nils = m_inst->nilModList();
    connect(nils, &ModFolderModel::updateFinished, this, &ScanModFolders::nilModsDone, Qt::UniqueConnection);
    if (!nils->update()) {
        m_nilModsDone = true;
    }
    checkDone();
}

void ScanModFolders::modsDone()
{
    qDebug() << "Check done in ScanModFolders modsDone...";
    if (!isRunning()) {
        qDebug() << "ScanModFolders::modsDone called but step not running; ignoring.";
        return;
    }
    m_modsDone = true;
    checkDone();
}

void ScanModFolders::coreModsDone()
{
    qDebug() << "Check done in ScanModFolders coreModsDone...";
    if (!isRunning()) {
        qDebug() << "ScanModFolders::coreModsDone called but step not running; "
                    "ignoring.";
        return;
    }
    m_coreModsDone = true;
    checkDone();
}

void ScanModFolders::nilModsDone()
{
    qDebug() << "Check done in ScanModFolders nilModsDone...";
    if (!isRunning()) {
        qDebug() << "ScanModFolders::nilModsDone called but step not running; ignoring.";
        return;
    }
    m_nilModsDone = true;
    checkDone();
}

void ScanModFolders::checkDone()
{
    qDebug() << "Check done in ScanModFolders...";
    if (m_modsDone && m_coreModsDone && m_nilModsDone) {
        // Disconnect model signals before finishing so the finished step doesn't
        // receive later updateFinished() notifications and try to finish again.
        auto m_inst = m_parent->instance();
        if (m_inst) {
            disconnect(m_inst->loaderModList(), &ModFolderModel::updateFinished, this, &ScanModFolders::modsDone);
            disconnect(m_inst->coreModList(), &ModFolderModel::updateFinished, this, &ScanModFolders::coreModsDone);
            disconnect(m_inst->nilModList(), &ModFolderModel::updateFinished, this, &ScanModFolders::nilModsDone);
        }

        emitSucceeded();
    }
}

void ScanModFolders::finalize()
{
    auto m_inst = m_parent->instance();
    if (!m_inst) {
        return;
    }

    disconnect(m_inst->loaderModList(), &ModFolderModel::updateFinished, this, &ScanModFolders::modsDone);
    disconnect(m_inst->coreModList(), &ModFolderModel::updateFinished, this, &ScanModFolders::coreModsDone);
    disconnect(m_inst->nilModList(), &ModFolderModel::updateFinished, this, &ScanModFolders::nilModsDone);
}
