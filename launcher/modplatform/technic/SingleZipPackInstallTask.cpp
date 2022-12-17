/* Copyright 2013-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "SingleZipPackInstallTask.h"

#include <QtConcurrent>

#include "MMCZip.h"
#include "TechnicPackProcessor.h"
#include "FileSystem.h"

#include "Application.h"

Technic::SingleZipPackInstallTask::SingleZipPackInstallTask(const QUrl &sourceUrl, const QString &minecraftVersion)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sourceUrl = sourceUrl;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minecraftVersion = minecraftVersion;
}

bool Technic::SingleZipPackInstallTask::abort() {
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_abortable)
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob->abort();
    }
    return false;
}

void Technic::SingleZipPackInstallTask::executeTask()
{
    setStatus(tr("Downloading modpack:\n%1").arg(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sourceUrl.toString()));

    const QString path = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sourceUrl.host() + '/' + hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sourceUrl.path();
    auto entry = APPLICATION->metacache()->resolveEntry("general", path);
    entry->setStale(true);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob = new NetJob(tr("Modpack download"), APPLICATION->network());
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob->addNetAction(Net::Download::makeCached(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sourceUrl, entry));
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_archivePath = entry->getFullPath();
    auto job = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob.get();
    connect(job, &NetJob::succeeded, this, &Technic::SingleZipPackInstallTask::downloadSucceeded);
    connect(job, &NetJob::progress, this, &Technic::SingleZipPackInstallTask::downloadProgressChanged);
    connect(job, &NetJob::failed, this, &Technic::SingleZipPackInstallTask::downloadFailed);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob->start();
}

void Technic::SingleZipPackInstallTask::downloadSucceeded()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_abortable = false;

    setStatus(tr("Extracting modpack"));
    QDir extractDir(FS::PathCombine(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stagingPath, ".minecraft"));
    qDebug() << "Attempting to create instance from" << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_archivePath;

    // open the zip and find relevant files in it
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_packZip.reset(new QuaZip(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_archivePath));
    if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_packZip->open(QuaZip::mdUnzip))
    {
        emitFailed(tr("Unable to open supplied modpack zip file."));
        return;
    }
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_extractFuture = QtConcurrent::run(QThreadPool::globalInstance(), MMCZip::extractSubDir, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_packZip.get(), QString(""), extractDir.absolutePath());
    connect(&hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_extractFutureWatcher, &QFutureWatcher<QStringList>::finished, this, &Technic::SingleZipPackInstallTask::extractFinished);
    connect(&hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_extractFutureWatcher, &QFutureWatcher<QStringList>::canceled, this, &Technic::SingleZipPackInstallTask::extractAborted);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_extractFutureWatcher.setFuture(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_extractFuture);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob.reset();
}

void Technic::SingleZipPackInstallTask::downloadFailed(QString reason)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_abortable = false;
    emitFailed(reason);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob.reset();
}

void Technic::SingleZipPackInstallTask::downloadProgressChanged(qint64 current, qint64 total)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_abortable = true;
    setProgress(current / 2, total);
}

void Technic::SingleZipPackInstallTask::extractFinished()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_packZip.reset();
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
        if (file.isDir())
        {
            // Folder +rwx for current user
            permissions |= QFileDevice::Permission::ReadUser | QFileDevice::Permission::WriteUser | QFileDevice::Permission::ExeUser;
        }
        else
        {
            // File +rw for current user
            permissions |= QFileDevice::Permission::ReadUser | QFileDevice::Permission::WriteUser;
        }
        if (origPermissions != permissions)
        {
            if (!QFile::setPermissions(filepath, permissions))
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
    connect(packProcessor.get(), &Technic::TechnicPackProcessor::succeeded, this, &Technic::SingleZipPackInstallTask::emitSucceeded);
    connect(packProcessor.get(), &Technic::TechnicPackProcessor::failed, this, &Technic::SingleZipPackInstallTask::emitFailed);
    packProcessor->run(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_globalSettings, name(), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instIcon, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stagingPath, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minecraftVersion);
}

void Technic::SingleZipPackInstallTask::extractAborted()
{
    emitFailed(tr("Instance import has been aborted."));
}
