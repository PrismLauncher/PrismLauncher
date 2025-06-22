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

#include "ModMinecraftJar.h"
#include "FileSystem.h"
#include "MMCZip.h"
#include "launch/LaunchTask.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

void ModMinecraftJar::executeTask()
{
    auto m_inst = m_parent->instance();

    if (!m_inst->getJarMods().size()) {
        emitSucceeded();
        return;
    }
    // nuke obsolete stripped jar(s) if needed
    if (!FS::ensureFolderPathExists(m_inst->binRoot())) {
        emitFailed(tr("Couldn't create the bin folder for Minecraft.jar"));
    }

    auto finalJarPath = QDir(m_inst->binRoot()).absoluteFilePath("minecraft.jar");
    if (!removeJar()) {
        emitFailed(tr("Couldn't remove stale jar file: %1").arg(finalJarPath));
    }

    // create temporary modded jar, if needed
    auto components = m_inst->getPackProfile();
    auto profile = components->getProfile();
    auto jarMods = m_inst->getJarMods();
    if (jarMods.size()) {
        auto mainJar = profile->getMainJar();
        QStringList jars, temp1, temp2, temp3, temp4;
        mainJar->getApplicableFiles(m_inst->runtimeContext(), jars, temp1, temp2, temp3, m_inst->getLocalLibraryPath());
        auto sourceJarPath = jars[0];
        if (!MMCZip::createModdedJar(sourceJarPath, finalJarPath, jarMods)) {
            emitFailed(tr("Failed to create the custom Minecraft jar file."));
            return;
        }
    }
    emitSucceeded();
}

void ModMinecraftJar::finalize()
{
    removeJar();
}

bool ModMinecraftJar::removeJar()
{
    auto m_inst = m_parent->instance();
    auto finalJarPath = QDir(m_inst->binRoot()).absoluteFilePath("minecraft.jar");
    QFile finalJar(finalJarPath);
    if (finalJar.exists()) {
        if (!finalJar.remove()) {
            return false;
        }
    }
    return true;
}
