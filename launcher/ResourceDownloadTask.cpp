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

#include "FileSystem.h"
#include "minecraft/mod/ResourceFolderModel.h"

#include "minecraft/mod/ShaderPackFolderModel.h"
#include "modplatform/helpers/HashUtils.h"
#include "net/ApiDownload.h"
#include "net/ChecksumValidator.h"

ResourceDownloadTask::ResourceDownloadTask(ModPlatform::IndexedPack::Ptr pack,
                                           ModPlatform::IndexedVersion version,
                                           ResourceFolderModel* packs,
                                           bool is_indexed)
    : m_pack(std::move(pack)), m_pack_version(std::move(version)), m_pack_model(packs)
{
    if (is_indexed) {
        m_update_task.reset(new LocalResourceUpdateTask(m_pack_model->indexDir(), *m_pack, m_pack_version));
        connect(m_update_task.get(), &LocalResourceUpdateTask::hasOldResource, this, &ResourceDownloadTask::hasOldResource);

        addTask(m_update_task);
    }

    m_filesNetJob.reset(new NetJob(tr("Resource download"), APPLICATION->network()));
    m_filesNetJob->setStatus(tr("Downloading resource:\n%1").arg(m_pack_version.downloadUrl));

    auto action = Net::ApiDownload::makeFile(m_pack_version.downloadUrl, m_pack_model->dir().absoluteFilePath(getFilename()));
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
    auto oldName = std::get<0>(to_delete);
    auto oldFilename = std::get<1>(to_delete);

    if (oldName.isEmpty() || oldFilename == m_pack_version.fileName)
        return;

    m_pack_model->uninstallResource(oldFilename, true);

    // also rename the shader config file
    if (dynamic_cast<ShaderPackFolderModel*>(m_pack_model) != nullptr) {
        QFileInfo oldConfig(m_pack_model->dir(), oldFilename + ".txt");
        QFileInfo newConfig(m_pack_model->dir(), getFilename() + ".txt");

        if (oldConfig.exists() && !newConfig.exists()) {
            bool success = FS::move(oldConfig.filePath(), newConfig.filePath());

            if (!success)
                emit logWarning(tr("Failed to rename shader config from '%1' to '%2'").arg(oldConfig.fileName(), newConfig.fileName()));
        }
    }
}

void ResourceDownloadTask::downloadFailed(QString reason)
{
    m_filesNetJob.reset();
    emitFailed(reason);
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
