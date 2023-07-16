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
    m_sourceUrl = sourceUrl;
    m_minecraftVersion = minecraftVersion;
}

bool Technic::SingleZipPackInstallTask::abort() {
    if(m_abortable)
    {
        return m_filesNetJob->abort();
    }
    return false;
}

void Technic::SingleZipPackInstallTask::executeTask()
{
    setStatus(tr("Downloading modpack:\n%1").arg(m_sourceUrl.toString()));

    const QString path = m_sourceUrl.host() + '/' + m_sourceUrl.path();
    auto entry = APPLICATION->metacache()->resolveEntry("general", path);
    entry->setStale(true);
    m_filesNetJob.reset(new NetJob(tr("Modpack download"), APPLICATION->network()));
    m_filesNetJob->addNetAction(Net::Download::makeCached(m_sourceUrl, entry));
    m_archivePath = entry->getFullPath();
    auto job = m_filesNetJob.get();
    connect(job, &NetJob::succeeded, this, &Technic::SingleZipPackInstallTask::downloadSucceeded);
    connect(job, &NetJob::progress, this, &Technic::SingleZipPackInstallTask::downloadProgressChanged);
    connect(job, &NetJob::stepProgress, this, &Technic::SingleZipPackInstallTask::propogateStepProgress);
    connect(job, &NetJob::failed, this, &Technic::SingleZipPackInstallTask::downloadFailed);
    m_filesNetJob->start();
}

void Technic::SingleZipPackInstallTask::downloadSucceeded()
{
    m_abortable = false;

    setStatus(tr("Extracting modpack"));
    QDir extractDir(FS::PathCombine(m_stagingPath, ".minecraft"));
    qDebug() << "Attempting to create instance from" << m_archivePath;

    // open the zip and find relevant files in it
    m_packZip.reset(new QuaZip(m_archivePath));
    if (!m_packZip->open(QuaZip::mdUnzip))
    {
        emitFailed(tr("Unable to open supplied modpack zip file."));
        return;
    }
    m_extractFuture = QtConcurrent::run(QThreadPool::globalInstance(), MMCZip::extractSubDir, m_packZip.get(), QString(""), extractDir.absolutePath());
    connect(&m_extractFutureWatcher, &QFutureWatcher<QStringList>::finished, this, &Technic::SingleZipPackInstallTask::extractFinished);
    connect(&m_extractFutureWatcher, &QFutureWatcher<QStringList>::canceled, this, &Technic::SingleZipPackInstallTask::extractAborted);
    m_extractFutureWatcher.setFuture(m_extractFuture);
    m_filesNetJob.reset();
}

void Technic::SingleZipPackInstallTask::downloadFailed(QString reason)
{
    m_abortable = false;
    emitFailed(reason);
    m_filesNetJob.reset();
}

void Technic::SingleZipPackInstallTask::downloadProgressChanged(qint64 current, qint64 total)
{
    m_abortable = true;
    setProgress(current / 2, total);
}

void Technic::SingleZipPackInstallTask::extractFinished()
{
    m_packZip.reset();
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

    auto packProcessor = makeShared<Technic::TechnicPackProcessor>();
    connect(packProcessor.get(), &Technic::TechnicPackProcessor::succeeded, this, &Technic::SingleZipPackInstallTask::emitSucceeded);
    connect(packProcessor.get(), &Technic::TechnicPackProcessor::failed, this, &Technic::SingleZipPackInstallTask::emitFailed);
    packProcessor->run(m_globalSettings, name(), m_instIcon, m_stagingPath, m_minecraftVersion);
}

void Technic::SingleZipPackInstallTask::extractAborted()
{
    emitFailed(tr("Instance import has been aborted."));
}
