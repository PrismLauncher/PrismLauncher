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

#include "Application.h"
#include "FileSystem.h"
#include "minecraft/mod/MetadataHandler.h"

#include <QThread>

ModFolderLoadTask::ModFolderLoadTask(QDir mods_dir, QDir index_dir, bool is_indexed, bool clean_orphan)
    : Task(nullptr, false)
    , m_mods_dir(mods_dir)
    , m_index_dir(index_dir)
    , m_is_indexed(is_indexed)
    , m_clean_orphan(clean_orphan)
    , m_result(new Result())
    , m_thread_to_spawn_into(thread())
{}

void ModFolderLoadTask::executeTask()
{
    if (thread() != m_thread_to_spawn_into)
        connect(this, &Task::finished, this->thread(), &QThread::quit);

    if (m_is_indexed) {
        // Read metadata first
        getFromMetadata();
    }

    // Read JAR files that don't have metadata
    m_mods_dir.refresh();
    for (auto entry : m_mods_dir.entryInfoList()) {
        auto filePath = entry.absoluteFilePath();
        if (auto app = APPLICATION_DYN; app && app->checkQSavePath(filePath)) {
            continue;
        }
        auto newFilePath = FS::getUniqueResourceName(filePath);
        if (newFilePath != filePath) {
            FS::move(filePath, newFilePath);
            entry = QFileInfo(newFilePath);
        }
        Mod* mod(new Mod(entry));

        if (mod->enabled()) {
            if (m_result->mods.contains(mod->internal_id())) {
                m_result->mods[mod->internal_id()]->setStatus(ModStatus::Installed);
                // Delete the object we just created, since a valid one is already in the mods list.
                delete mod;
            } else {
                m_result->mods[mod->internal_id()].reset(std::move(mod));
                m_result->mods[mod->internal_id()]->setStatus(ModStatus::NoMetadata);
            }
        } else {
            QString chopped_id = mod->internal_id().chopped(9);
            if (m_result->mods.contains(chopped_id)) {
                m_result->mods[mod->internal_id()].reset(std::move(mod));

                auto metadata = m_result->mods[chopped_id]->metadata();
                if (metadata) {
                    mod->setMetadata(*metadata);

                    m_result->mods[mod->internal_id()]->setStatus(ModStatus::Installed);
                    m_result->mods.remove(chopped_id);
                }
            } else {
                m_result->mods[mod->internal_id()].reset(std::move(mod));
                m_result->mods[mod->internal_id()]->setStatus(ModStatus::NoMetadata);
            }
        }
    }

    // Remove orphan metadata to prevent issues
    // See https://github.com/PolyMC/PolyMC/issues/996
    if (m_clean_orphan) {
        QMutableMapIterator iter(m_result->mods);
        while (iter.hasNext()) {
            auto mod = iter.next().value();
            if (mod->status() == ModStatus::NotInstalled) {
                mod->destroy(m_index_dir, false, false);
                iter.remove();
            }
        }
    }

    for (auto mod : m_result->mods)
        mod->moveToThread(m_thread_to_spawn_into);

    if (m_aborted)
        emit finished();
    else
        emitSucceeded();
}

void ModFolderLoadTask::getFromMetadata()
{
    m_index_dir.refresh();
    for (auto entry : m_index_dir.entryList(QDir::Files)) {
        auto metadata = Metadata::get(m_index_dir, entry);

        if (!metadata.isValid()) {
            continue;
        }

        auto* mod = new Mod(m_mods_dir, metadata);
        mod->setStatus(ModStatus::NotInstalled);
        m_result->mods[mod->internal_id()].reset(std::move(mod));
    }
}
