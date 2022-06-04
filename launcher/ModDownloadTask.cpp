// SPDX-License-Identifier: GPL-3.0-only
/*
*  PolyMC - Minecraft Launcher
*  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
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
    : m_mod(mod), m_mod_version(version), mods(mods)
{
    if (is_indexed) {
        m_update_task.reset(new LocalModUpdateTask(mods->indexDir(), m_mod, m_mod_version));

        addTask(m_update_task);
    }

    m_filesNetJob.reset(new NetJob(tr("Mod download"), APPLICATION->network()));
    m_filesNetJob->setStatus(tr("Downloading mod:\n%1").arg(m_mod_version.downloadUrl));
    
    m_filesNetJob->addNetAction(Net::Download::makeFile(m_mod_version.downloadUrl, mods->dir().absoluteFilePath(getFilename())));
    connect(m_filesNetJob.get(), &NetJob::succeeded, this, &ModDownloadTask::downloadSucceeded);
    connect(m_filesNetJob.get(), &NetJob::progress, this, &ModDownloadTask::downloadProgressChanged);
    connect(m_filesNetJob.get(), &NetJob::failed, this, &ModDownloadTask::downloadFailed);

    addTask(m_filesNetJob);

}

void ModDownloadTask::downloadSucceeded()
{
    m_filesNetJob.reset();
}

void ModDownloadTask::downloadFailed(QString reason)
{
    emitFailed(reason);
    m_filesNetJob.reset();
}

void ModDownloadTask::downloadProgressChanged(qint64 current, qint64 total)
{
    emit progress(current, total);
}
