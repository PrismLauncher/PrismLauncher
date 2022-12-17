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
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_solderUrl = solderUrl;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pack = pack;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version = version;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_network = network;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minecraftVersion = minecraftVersion;
}

bool Technic::SolderPackInstallTask::abort() {
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_abortable)
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob->abort();
    }
    return false;
}

void Technic::SolderPackInstallTask::executeTask()
{
    setStatus(tr("Resolving modpack files"));

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob = new NetJob(tr("Resolving modpack files"), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_network);
    auto sourceUrl = QString("%1/modpack/%2/%3").arg(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_solderUrl.toString(), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pack, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob->addNetAction(Net::Download::makeByteArray(sourceUrl, &hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_response));

    auto job = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob.get();
    connect(job, &NetJob::succeeded, this, &Technic::SolderPackInstallTask::fileListSucceeded);
    connect(job, &NetJob::failed, this, &Technic::SolderPackInstallTask::downloadFailed);
    connect(job, &NetJob::aborted, this, &Technic::SolderPackInstallTask::downloadAborted);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob->start();
}

void Technic::SolderPackInstallTask::fileListSucceeded()
{
    setStatus(tr("Downloading modpack"));

    QJsonParseError parse_error {};
    QJsonDocument doc = QJsonDocument::fromJson(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_response, &parse_error);
    if (parse_error.error != QJsonParseError::NoError) {
        qWarning() << "Error while parsing JSON response from Solder at " << parse_error.offset << " reason: " << parse_error.errorString();
        qWarning() << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_response;
        return;
    }
    auto obj = doc.object();

    TechnicSolder::PackBuild build;
    try {
        TechnicSolder::loadPackBuild(build, obj);
    }
    catch (const JSONValidationError& e) {
        emitFailed(tr("Could not understand pack manifest:\n") + e.cause());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob.reset();
        return;
    }

    if (!build.minecraft.isEmpty())
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minecraftVersion = build.minecraft;

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob = new NetJob(tr("Downloading modpack"), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_network);

    int i = 0;
    for (const auto &mod : build.mods) {
        auto path = FS::PathCombine(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_outputDir.path(), QString("%1").arg(i));

        auto dl = Net::Download::makeFile(mod.url, path);
        if (!mod.md5.isEmpty()) {
            auto rawMd5 = QByteArray::fromHex(mod.md5.toLatin1());
            dl->addValidator(new Net::ChecksumValidator(QCryptographicHash::Md5, rawMd5));
        }
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob->addNetAction(dl);

        i++;
    }

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_modCount = build.mods.size();

    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob.get(), &NetJob::succeeded, this, &Technic::SolderPackInstallTask::downloadSucceeded);
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob.get(), &NetJob::progress, this, &Technic::SolderPackInstallTask::downloadProgressChanged);
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob.get(), &NetJob::failed, this, &Technic::SolderPackInstallTask::downloadFailed);
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob.get(), &NetJob::aborted, this, &Technic::SolderPackInstallTask::downloadAborted);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob->start();
}

void Technic::SolderPackInstallTask::downloadSucceeded()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_abortable = false;

    setStatus(tr("Extracting modpack"));
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob.reset();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_extractFuture = QtConcurrent::run([this]()
    {
        int i = 0;
        QString extractDir = FS::PathCombine(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stagingPath, ".minecraft");
        FS::ensureFolderPathExists(extractDir);

        while (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_modCount > i)
        {
            auto path = FS::PathCombine(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_outputDir.path(), QString("%1").arg(i));
            if (!MMCZip::extractDir(path, extractDir))
            {
                return false;
            }
            i++;
        }
        return true;
    });
    connect(&hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_extractFutureWatcher, &QFutureWatcher<QStringList>::finished, this, &Technic::SolderPackInstallTask::extractFinished);
    connect(&hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_extractFutureWatcher, &QFutureWatcher<QStringList>::canceled, this, &Technic::SolderPackInstallTask::extractAborted);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_extractFutureWatcher.setFuture(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_extractFuture);
}

void Technic::SolderPackInstallTask::downloadFailed(QString reason)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_abortable = false;
    emitFailed(reason);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob.reset();
}

void Technic::SolderPackInstallTask::downloadProgressChanged(qint64 current, qint64 total)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_abortable = true;
    setProgress(current / 2, total);
}

void Technic::SolderPackInstallTask::downloadAborted()
{
    emitAborted();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob.reset();
}

void Technic::SolderPackInstallTask::extractFinished()
{
    if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_extractFuture.result())
    {
        emitFailed(tr("Failed to extract modpack"));
        return;
    }
    QDir extractDir(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stagingPath);

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

    shared_qobject_ptr<Technic::TechnicPackProcessor> packProcessor = new Technic::TechnicPackProcessor();
    connect(packProcessor.get(), &Technic::TechnicPackProcessor::succeeded, this, &Technic::SolderPackInstallTask::emitSucceeded);
    connect(packProcessor.get(), &Technic::TechnicPackProcessor::failed, this, &Technic::SolderPackInstallTask::emitFailed);
    packProcessor->run(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_globalSettings, name(), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instIcon, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stagingPath, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minecraftVersion, true);
}

void Technic::SolderPackInstallTask::extractAborted()
{
    emitFailed(tr("Instance import has been aborted."));
}

