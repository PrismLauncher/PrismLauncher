// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Octol1ttle <l1ttleofficial@outlook.com>
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

#include "EnsureOfflineLibraries.h"

#include "minecraft/PackProfile.h"

EnsureOfflineLibraries::EnsureOfflineLibraries(LaunchTask* parent, MinecraftInstance* instance) : LaunchStep(parent), m_instance(instance)
{}

void EnsureOfflineLibraries::executeTask()
{
    const auto profile = m_instance->getPackProfile()->getProfile();
    QStringList allJars;
    profile->getLibraryFiles(m_instance->runtimeContext(), allJars, allJars, m_instance->getLocalLibraryPath(), m_instance->binRoot());
    for (const auto& jar : allJars) {
        if (!QFileInfo::exists(jar)) {
            emit logLine(tr("This instance cannot be launched because some libraries are missing or have not been downloaded yet. Please "
                            "try again in online mode with a working Internet connection"),
                         MessageLevel::Fatal);
            emitFailed("Required libraries are missing");
            return;
        }
    }

    emitSucceeded();
}
