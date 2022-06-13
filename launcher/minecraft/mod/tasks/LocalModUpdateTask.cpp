// SPDX-License-Identifier: GPL-3.0-only
/*
*  PolyMC - Minecraft Launcher
*  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
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
*/

#include "LocalModUpdateTask.h"

#include "Application.h"
#include "FileSystem.h"
#include "minecraft/mod/MetadataHandler.h"

LocalModUpdateTask::LocalModUpdateTask(QDir index_dir, ModPlatform::IndexedPack& mod, ModPlatform::IndexedVersion& mod_version)
    : m_index_dir(index_dir), m_mod(mod), m_mod_version(mod_version)
{
    // Ensure a '.index' folder exists in the mods folder, and create it if it does not
    if (!FS::ensureFolderPathExists(index_dir.path())) {
        emitFailed(QString("Unable to create index for mod %1!").arg(m_mod.name));
    }
}

void LocalModUpdateTask::executeTask()
{
    setStatus(tr("Updating index for mod:\n%1").arg(m_mod.name));

    auto pw_mod = Metadata::create(m_index_dir, m_mod, m_mod_version);
    Metadata::update(m_index_dir, pw_mod);

    emitSucceeded();
}

auto LocalModUpdateTask::abort() -> bool
{
    emitAborted();
    return true;
}
