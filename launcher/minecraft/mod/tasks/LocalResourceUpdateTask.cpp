// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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

#include "LocalResourceUpdateTask.h"

#include "FileSystem.h"
#include "minecraft/mod/MetadataHandler.h"

#ifdef Q_OS_WIN32
#include <windows.h>
#endif

LocalResourceUpdateTask::LocalResourceUpdateTask(const QDir& indexDir,
                                                 ModPlatform::IndexedPack project,
                                                 ModPlatform::IndexedVersion version,
                                                 bool lockUpdate)
    : m_indexDir(indexDir), m_project(std::move(project)), m_version(std::move(version)), m_lockUpdate(lockUpdate)
{
    // Ensure a '.index' folder exists in the mods folder, and create it if it does not
    if (!FS::ensureFolderPathExists(indexDir.path())) {
        emitFailed(QString("Unable to create index directory at %1!").arg(indexDir.absolutePath()));
        return;
    }

#ifdef Q_OS_WIN32
    std::wstring wpath = indexDir.path().toStdWString();
    if (indexDir.dirName().startsWith('.')) {
        SetFileAttributesW(wpath.c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED);
    } else {
        // fix shaderpacks folder being hidden by Prism Launcher 10.0.1
        SetFileAttributesW(wpath.c_str(), FILE_ATTRIBUTE_NORMAL);
    }
#endif
}

void LocalResourceUpdateTask::executeTask()
{
    setStatus(tr("Updating index for resource:\n%1").arg(m_project.name));

    auto oldMetadata = Metadata::get(m_indexDir, m_project.addonId);
    if (oldMetadata.isValid()) {
        emit hasOldResource(oldMetadata.name, oldMetadata.filename);
        if (m_project.slug.isEmpty()) {
            m_project.slug = oldMetadata.slug;
        }
    }

    auto pwMod = Metadata::create(m_indexDir, m_project, m_version);
    if (pwMod.isValid()) {
        pwMod.lockUpdate = m_lockUpdate;
        Metadata::update(m_indexDir, pwMod);
        emitSucceeded();
    } else {
        qCritical() << "Tried to update an invalid resource!";
        emitFailed(tr("Invalid metadata"));
    }
}

auto LocalResourceUpdateTask::abort() -> bool
{
    emitAborted();
    return true;
}
