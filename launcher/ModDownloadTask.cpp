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

#include "ModDownloadTask.h"

#include "Application.h"
#include "minecraft/mod/ModFolderModel.h"

ModDownloadTask::ModDownloadTask(ModPlatform::IndexedPack mod, ModPlatform::IndexedVersion version, const std::shared_ptr<ModFolderModel> mods, bool is_indexed)
    : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mod(mod), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mod_version(version), mods(mods)
{
    if (is_indexed) {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_update_task.reset(new LocalModUpdateTask(mods->indexDir(), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mod, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mod_version));
        connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_update_task.get(), &LocalModUpdateTask::hasOldMod, this, &ModDownloadTask::hasOldMod);

        addTask(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_update_task);
    }

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob.reset(new NetJob(tr("Mod download"), APPLICATION->network()));
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob->setStatus(tr("Downloading mod:\n%1").arg(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mod_version.downloadUrl));
    
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob->addNetAction(Net::Download::makeFile(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mod_version.downloadUrl, mods->dir().absoluteFilePath(getFilename())));
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob.get(), &NetJob::succeeded, this, &ModDownloadTask::downloadSucceeded);
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob.get(), &NetJob::progress, this, &ModDownloadTask::downloadProgressChanged);
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob.get(), &NetJob::failed, this, &ModDownloadTask::downloadFailed);

    addTask(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob);
}

void ModDownloadTask::downloadSucceeded()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob.reset();
    auto name = std::get<0>(to_delete);
    auto filename = std::get<1>(to_delete);
    if (!name.isEmpty() && filename != hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mod_version.fileName) {
        mods->uninstallMod(filename, true);
    }
}

void ModDownloadTask::downloadFailed(QString reason)
{
    emitFailed(reason);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob.reset();
}

void ModDownloadTask::downloadProgressChanged(qint64 current, qint64 total)
{
    emit progress(current, total);
}

// This indirection is done so that we don't delete a mod before being sure it was
// downloaded successfully!
void ModDownloadTask::hasOldMod(QString name, QString filename)
{
    to_delete = {name, filename};
}
