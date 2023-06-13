// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (c) 2021-2022 Jamie Mansfield <jmansfield@cadixdev.org>
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

#include "SolderPackInstallTask.h"

#include <FileSystem.h>
#include <Json.h>
#include <QtConcurrentRun>
#include <MMCZip.h>

#include "TechnicPackProcessor.h"
#include "SolderPackManifest.h"
#include "net/ChecksumValidator.h"

Technic::SolderPackInstallTask::SolderPackInstallTask(
    shared_qobject_ptr<QNetworkAccessManager> network,
    const QUrl &solderUrl,
    const QString &pack,
    const QString &version,
    const QString &minecraftVersion
) {
    m_solderUrl = solderUrl;
    m_pack = pack;
    m_version = version;
    m_network = network;
    m_minecraftVersion = minecraftVersion;
}

bool Technic::SolderPackInstallTask::abort() {
    if(m_abortable)
    {
        return m_filesNetJob->abort();
    }
    return false;
}

void Technic::SolderPackInstallTask::executeTask()
{
    setStatus(tr("Resolving modpack files"));

    m_filesNetJob.reset(new NetJob(tr("Resolving modpack files"), m_network));
    auto sourceUrl = QString("%1/modpack/%2/%3").arg(m_solderUrl.toString(), m_pack, m_version);
    m_filesNetJob->addNetAction(Net::Download::makeByteArray(sourceUrl, &m_response));

    auto job = m_filesNetJob.get();
    connect(job, &NetJob::succeeded, this, &Technic::SolderPackInstallTask::fileListSucceeded);
    connect(job, &NetJob::failed, this, &Technic::SolderPackInstallTask::downloadFailed);
    connect(job, &NetJob::aborted, this, &Technic::SolderPackInstallTask::downloadAborted);
    m_filesNetJob->start();
}

void Technic::SolderPackInstallTask::fileListSucceeded()
{
    setStatus(tr("Downloading modpack"));

    QJsonParseError parse_error {};
    QJsonDocument doc = QJsonDocument::fromJson(m_response, &parse_error);
    if (parse_error.error != QJsonParseError::NoError) {
        qWarning() << "Error while parsing JSON response from Solder at " << parse_error.offset << " reason: " << parse_error.errorString();
        qWarning() << m_response;
        return;
    }
    auto obj = doc.object();

    TechnicSolder::PackBuild build;
    try {
        TechnicSolder::loadPackBuild(build, obj);
    }
    catch (const JSONValidationError& e) {
        emitFailed(tr("Could not understand pack manifest:\n") + e.cause());
        m_filesNetJob.reset();
        return;
    }

    if (!build.minecraft.isEmpty())
        m_minecraftVersion = build.minecraft;

    m_filesNetJob.reset(new NetJob(tr("Downloading modpack"), m_network));

    int i = 0;
    for (const auto &mod : build.mods) {
        auto path = FS::PathCombine(m_outputDir.path(), QString("%1").arg(i));

        auto dl = Net::Download::makeFile(mod.url, path);
        if (!mod.md5.isEmpty()) {
            auto rawMd5 = QByteArray::fromHex(mod.md5.toLatin1());
            dl->addValidator(new Net::ChecksumValidator(QCryptographicHash::Md5, rawMd5));
        }
        m_filesNetJob->addNetAction(dl);

        i++;
    }

    m_modCount = build.mods.size();

    connect(m_filesNetJob.get(), &NetJob::succeeded, this, &Technic::SolderPackInstallTask::downloadSucceeded);
    connect(m_filesNetJob.get(), &NetJob::progress, this, &Technic::SolderPackInstallTask::downloadProgressChanged);
    connect(m_filesNetJob.get(), &NetJob::stepProgress, this, &Technic::SolderPackInstallTask::propogateStepProgress);
    connect(m_filesNetJob.get(), &NetJob::failed, this, &Technic::SolderPackInstallTask::downloadFailed);
    connect(m_filesNetJob.get(), &NetJob::aborted, this, &Technic::SolderPackInstallTask::downloadAborted);
    m_filesNetJob->start();
}

void Technic::SolderPackInstallTask::downloadSucceeded()
{
    m_abortable = false;

    setStatus(tr("Extracting modpack"));
    m_filesNetJob.reset();
    m_extractFuture = QtConcurrent::run([this]()
    {
        int i = 0;
        QString extractDir = FS::PathCombine(m_stagingPath, ".minecraft");
        FS::ensureFolderPathExists(extractDir);

        while (m_modCount > i)
        {
            auto path = FS::PathCombine(m_outputDir.path(), QString("%1").arg(i));
            if (!MMCZip::extractDir(path, extractDir))
            {
                return false;
            }
            i++;
        }
        return true;
    });
    connect(&m_extractFutureWatcher, &QFutureWatcher<QStringList>::finished, this, &Technic::SolderPackInstallTask::extractFinished);
    connect(&m_extractFutureWatcher, &QFutureWatcher<QStringList>::canceled, this, &Technic::SolderPackInstallTask::extractAborted);
    m_extractFutureWatcher.setFuture(m_extractFuture);
}

void Technic::SolderPackInstallTask::downloadFailed(QString reason)
{
    m_abortable = false;
    emitFailed(reason);
    m_filesNetJob.reset();
}

void Technic::SolderPackInstallTask::downloadProgressChanged(qint64 current, qint64 total)
{
    m_abortable = true;
    setProgress(current / 2, total);
}

void Technic::SolderPackInstallTask::downloadAborted()
{
    emitAborted();
    m_filesNetJob.reset();
}

void Technic::SolderPackInstallTask::extractFinished()
{
    if (!m_extractFuture.result())
    {
        emitFailed(tr("Failed to extract modpack"));
        return;
    }
    QDir extractDir(m_stagingPath);

    qDebug() << "Fixing permissions for extracted pack files...";
    QDirIterator it(extractDir, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        auto filepath = it.next();
        QFileInfo file(filepath);
        auto permissions = QFile::permissions(filepath);
        auto origPermissions = permissions;
        if(file.isDir())
        {
            // Folder +rwx for current user
            permissions |= QFileDevice::Permission::ReadUser | QFileDevice::Permission::WriteUser | QFileDevice::Permission::ExeUser;
        }
        else
        {
            // File +rw for current user
            permissions |= QFileDevice::Permission::ReadUser | QFileDevice::Permission::WriteUser;
        }
        if(origPermissions != permissions)
        {
            if(!QFile::setPermissions(filepath, permissions))
            {
                logWarning(tr("Could not fix permissions for %1").arg(filepath));
            }
            else
            {
                qDebug() << "Fixed" << filepath;
            }
        }
    }

    auto packProcessor = makeShared<Technic::TechnicPackProcessor>();
    connect(packProcessor.get(), &Technic::TechnicPackProcessor::succeeded, this, &Technic::SolderPackInstallTask::emitSucceeded);
    connect(packProcessor.get(), &Technic::TechnicPackProcessor::failed, this, &Technic::SolderPackInstallTask::emitFailed);
    packProcessor->run(m_globalSettings, name(), m_instIcon, m_stagingPath, m_minecraftVersion, true);
}

void Technic::SolderPackInstallTask::extractAborted()
{
    emitFailed(tr("Instance import has been aborted."));
}

