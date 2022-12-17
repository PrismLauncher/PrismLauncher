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

#include "ModFolderLoadTask.h"

#include "minecraft/mod/MetadataHandler.h"

#include <QThread>

ModFolderLoadTask::ModFolderLoadTask(QDir mods_dir, QDir index_dir, bool is_indexed, bool clean_orphan)
    : Task(nullptr, false)
    , hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mods_dir(mods_dir)
    , hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_index_dir(index_dir)
    , hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_is_indexed(is_indexed)
    , hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_clean_orphan(clean_orphan)
    , hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_result(new Result())
    , hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_thread_to_spawn_into(thread())
{}

void ModFolderLoadTask::executeTask()
{
    if (thread() != hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_thread_to_spawn_into)
        connect(this, &Task::finished, this->thread(), &QThread::quit);

    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_is_indexed) {
        // Read metadata first
        getFromMetadata();
    }

    // Read JAR files that don't have metadata
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mods_dir.refresh();
    for (auto entry : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mods_dir.entryInfoList()) {
        Mod* mod(new Mod(entry));

        if (mod->enabled()) {
            if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_result->mods.contains(mod->internal_id())) {
                hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_result->mods[mod->internal_id()]->setStatus(ModStatus::Installed);
                // Delete the object we just created, since a valid one is already in the mods list.
                delete mod;
            }
            else {
                hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_result->mods[mod->internal_id()] = mod;
                hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_result->mods[mod->internal_id()]->setStatus(ModStatus::NoMetadata);
            }
        }
        else { 
            QString chopped_id = mod->internal_id().chopped(9);
            if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_result->mods.contains(chopped_id)) {
                hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_result->mods[mod->internal_id()] = mod;

                auto metadata = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_result->mods[chopped_id]->metadata();
                if (metadata) {
                    mod->setMetadata(*metadata);

                    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_result->mods[mod->internal_id()]->setStatus(ModStatus::Installed);
                    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_result->mods.remove(chopped_id);
                }
            }
            else {
                hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_result->mods[mod->internal_id()] = mod;
                hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_result->mods[mod->internal_id()]->setStatus(ModStatus::NoMetadata);
            }
        }
    }

    // Remove orphan metadata to prevent issues
    // See https://github.com/PolyMC/PolyMC/issues/996
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_clean_orphan) {
        QMutableMapIterator iter(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_result->mods);
        while (iter.hasNext()) {
            auto mod = iter.next().value();
            if (mod->status() == ModStatus::NotInstalled) {
                mod->destroy(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_index_dir, false);
                iter.remove();
            }
        }
    }

    for (auto mod : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_result->mods)
        mod->moveToThread(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_thread_to_spawn_into);

    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_aborted)
        emit finished();
    else
        emitSucceeded();
}

void ModFolderLoadTask::getFromMetadata()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_index_dir.refresh();
    for (auto entry : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_index_dir.entryList(QDir::Files)) {
        auto metadata = Metadata::get(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_index_dir, entry);

        if(!metadata.isValid()){
            return;
        }

        auto* mod = new Mod(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mods_dir, metadata);
        mod->setStatus(ModStatus::NotInstalled);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_result->mods[mod->internal_id()] = mod;
    }
}
