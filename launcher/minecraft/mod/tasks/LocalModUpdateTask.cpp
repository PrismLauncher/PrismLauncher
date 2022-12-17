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

#ifdef Q_OS_WIN32
#include <windows.h>
#endif

LocalModUpdateTask::LocalModUpdateTask(QDir index_dir, ModPlatform::IndexedPack& mod, ModPlatform::IndexedVersion& mod_version)
    : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_index_dir(index_dir), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mod(mod), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mod_version(mod_version)
{
    // Ensure a '.index' folder exists in the mods folder, and create it if it does not
    if (!FS::ensureFolderPathExists(index_dir.path())) {
        emitFailed(QString("Unable to create index for mod %1!").arg(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mod.name));
    }

#ifdef Q_OS_WIN32
    SetFileAttributesW(index_dir.path().toStdWString().c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED);
#endif
}

void LocalModUpdateTask::executeTask()
{
    setStatus(tr("Updating index for mod:\n%1").arg(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mod.name));

    auto old_metadata = Metadata::get(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_index_dir, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mod.addonId);
    if (old_metadata.isValid()) {
        emit hasOldMod(old_metadata.name, old_metadata.filename);
        if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mod.slug.isEmpty())
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mod.slug = old_metadata.slug;
    }

    auto pw_mod = Metadata::create(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_index_dir, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mod, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mod_version);
    if (pw_mod.isValid()) {
        Metadata::update(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_index_dir, pw_mod);
        emitSucceeded();
    } else {
        qCritical() << "Tried to update an invalid mod!";
        emitFailed(tr("Invalid metadata"));
    }
}

auto LocalModUpdateTask::abort() -> bool
{
    emitAborted();
    return true;
}
