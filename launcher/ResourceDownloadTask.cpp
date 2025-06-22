// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022-2023 flowln <flowlnlnln@gmail.com>
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

#include "ResourceDownloadTask.h"

#include "Application.h"

#include "minecraft/mod/ModFolderModel.h"
#include "minecraft/mod/ResourceFolderModel.h"

#include "modplatform/helpers/HashUtils.h"
#include "net/ApiDownload.h"
#include "net/ChecksumValidator.h"

ResourceDownloadTask::ResourceDownloadTask(ModPlatform::IndexedPack::Ptr pack,
                                           ModPlatform::IndexedVersion version,
                                           const std::shared_ptr<ResourceFolderModel> packs,
                                           bool is_indexed,
                                           QString custom_target_folder)
    : m_pack(std::move(pack)), m_pack_version(std::move(version)), m_pack_model(packs), m_custom_target_folder(custom_target_folder)
{
    if (is_indexed) {
        m_update_task.reset(new LocalResourceUpdateTask(m_pack_model->indexDir(), *m_pack, m_pack_version));
        connect(m_update_task.get(), &LocalResourceUpdateTask::hasOldResource, this, &ResourceDownloadTask::hasOldResource);

        addTask(m_update_task);
    }

    m_filesNetJob.reset(new NetJob(tr("Resource download"), APPLICATION->network()));
    m_filesNetJob->setStatus(tr("Downloading resource:\n%1").arg(m_pack_version.downloadUrl));

    QDir dir{ m_pack_model->dir() };
    {
        // FIXME: Make this more generic. May require adding additional info to IndexedVersion,
        //        or adquiring a reference to the base instance.
        if (!m_custom_target_folder.isEmpty()) {
            dir.cdUp();
            dir.cd(m_custom_target_folder);
        }
    }

    auto action = Net::ApiDownload::makeFile(m_pack_version.downloadUrl, dir.absoluteFilePath(getFilename()));
    if (!m_pack_version.hash_type.isEmpty() && !m_pack_version.hash.isEmpty()) {
        switch (Hashing::algorithmFromString(m_pack_version.hash_type)) {
            case Hashing::Algorithm::Md4:
                action->addValidator(new Net::ChecksumValidator(QCryptographicHash::Algorithm::Md4, m_pack_version.hash));
                break;
            case Hashing::Algorithm::Md5:
                action->addValidator(new Net::ChecksumValidator(QCryptographicHash::Algorithm::Md5, m_pack_version.hash));
                break;
            case Hashing::Algorithm::Sha1:
                action->addValidator(new Net::ChecksumValidator(QCryptographicHash::Algorithm::Sha1, m_pack_version.hash));
                break;
            case Hashing::Algorithm::Sha256:
                action->addValidator(new Net::ChecksumValidator(QCryptographicHash::Algorithm::Sha256, m_pack_version.hash));
                break;
            case Hashing::Algorithm::Sha512:
                action->addValidator(new Net::ChecksumValidator(QCryptographicHash::Algorithm::Sha512, m_pack_version.hash));
                break;
            default:
                break;
        }
    }
    m_filesNetJob->addNetAction(action);
    connect(m_filesNetJob.get(), &NetJob::succeeded, this, &ResourceDownloadTask::downloadSucceeded);
    connect(m_filesNetJob.get(), &NetJob::progress, this, &ResourceDownloadTask::downloadProgressChanged);
    connect(m_filesNetJob.get(), &NetJob::stepProgress, this, &ResourceDownloadTask::propagateStepProgress);
    connect(m_filesNetJob.get(), &NetJob::failed, this, &ResourceDownloadTask::downloadFailed);

    addTask(m_filesNetJob);
}

void ResourceDownloadTask::downloadSucceeded()
{
    m_filesNetJob.reset();
    auto name = std::get<0>(to_delete);
    auto filename = std::get<1>(to_delete);
    if (!name.isEmpty() && filename != m_pack_version.fileName)
        m_pack_model->uninstallResource(filename, true);
}

void ResourceDownloadTask::downloadFailed(QString reason)
{
    emitFailed(reason);
    m_filesNetJob.reset();
}

void ResourceDownloadTask::downloadProgressChanged(qint64 current, qint64 total)
{
    emit progress(current, total);
}

// This indirection is done so that we don't delete a mod before being sure it was
// downloaded successfully!
void ResourceDownloadTask::hasOldResource(QString name, QString filename)
{
    to_delete = { name, filename };
}
