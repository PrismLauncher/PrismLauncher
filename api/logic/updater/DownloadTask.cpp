/* Copyright 2013-2019 MultiMC Contributors
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

#include "DownloadTask.h"

#include "updater/UpdateChecker.h"
#include "GoUpdate.h"
#include "net/NetJob.h"

#include <QFile>
#include <QTemporaryDir>
#include <QCryptographicHash>

namespace GoUpdate
{

DownloadTask::DownloadTask(Status status, QString target, QObject *parent)
    : Task(parent), m_updateFilesDir(target)
{
    m_status = status;

    m_updateFilesDir.setAutoRemove(false);
}

void DownloadTask::executeTask()
{
    loadVersionInfo();
}

void DownloadTask::loadVersionInfo()
{
    setStatus(tr("Loading version information..."));

    NetJob *netJob = new NetJob("Version Info");

    // Find the index URL.
    QUrl newIndexUrl = QUrl(m_status.newRepoUrl).resolved(QString::number(m_status.newVersionId) + ".json");
    qDebug() << m_status.newRepoUrl << " turns into " << newIndexUrl;

    netJob->addNetAction(m_newVersionFileListDownload = Net::Download::makeByteArray(newIndexUrl, &newVersionFileListData));

    // If we have a current version URL, get that one too.
    if (!m_status.currentRepoUrl.isEmpty())
    {
        QUrl cIndexUrl = QUrl(m_status.currentRepoUrl).resolved(QString::number(m_status.currentVersionId) + ".json");
        netJob->addNetAction(m_currentVersionFileListDownload = Net::Download::makeByteArray(cIndexUrl, &currentVersionFileListData));
        qDebug() << m_status.currentRepoUrl << " turns into " << cIndexUrl;
    }

    // connect signals and start the job
    connect(netJob, &NetJob::succeeded, this, &DownloadTask::processDownloadedVersionInfo);
    connect(netJob, &NetJob::failed, this, &DownloadTask::vinfoDownloadFailed);
    m_vinfoNetJob.reset(netJob);
    netJob->start();
}

void DownloadTask::vinfoDownloadFailed()
{
    // Something failed. We really need the second download (current version info), so parse
    // downloads anyways as long as the first one succeeded.
    if (m_newVersionFileListDownload->wasSuccessful())
    {
        processDownloadedVersionInfo();
        return;
    }

    // TODO: Give a more detailed error message.
    qCritical() << "Failed to download version info files.";
    emitFailed(tr("Failed to download version info files."));
}

void DownloadTask::processDownloadedVersionInfo()
{
    VersionFileList m_currentVersionFileList;
    VersionFileList m_newVersionFileList;

    setStatus(tr("Reading file list for new version..."));
    qDebug() << "Reading file list for new version...";
    QString error;
    if (!parseVersionInfo(newVersionFileListData, m_newVersionFileList, error))
    {
        qCritical() << error;
        emitFailed(error);
        return;
    }

    // if we have the current version info, use it.
    if (m_currentVersionFileListDownload && m_currentVersionFileListDownload->wasSuccessful())
    {
        setStatus(tr("Reading file list for current version..."));
        qDebug() << "Reading file list for current version...";
        // if this fails, it's not a complete loss.
        QString error;
        if(!parseVersionInfo( currentVersionFileListData, m_currentVersionFileList, error))
        {
            qDebug() << error << "This is not a fatal error.";
        }
    }

    // We don't need this any more.
    m_currentVersionFileListDownload.reset();
    m_newVersionFileListDownload.reset();
    m_vinfoNetJob.reset();

    setStatus(tr("Processing file lists - figuring out how to install the update..."));

    // make a new netjob for the actual update files
    NetJobPtr netJob (new NetJob("Update Files"));

    // fill netJob and operationList
    if (!processFileLists(m_currentVersionFileList, m_newVersionFileList, m_status.rootPath, m_updateFilesDir.path(), netJob, m_operations))
    {
        emitFailed(tr("Failed to process update lists..."));
        return;
    }

    // Now start the download.
    QObject::connect(netJob.get(), &NetJob::succeeded, this, &DownloadTask::fileDownloadFinished);
    QObject::connect(netJob.get(), &NetJob::progress, this, &DownloadTask::fileDownloadProgressChanged);
    QObject::connect(netJob.get(), &NetJob::failed, this, &DownloadTask::fileDownloadFailed);

    if(netJob->size() == 1) // Translation issues... see https://github.com/MultiMC/MultiMC5/issues/1701
    {
        setStatus(tr("Downloading one update file."));
    }
    else
    {
        setStatus(tr("Downloading %1 update files.").arg(QString::number(netJob->size())));
    }
    qDebug() << "Begin downloading update files to" << m_updateFilesDir.path();
    m_filesNetJob = netJob;
    m_filesNetJob->start();
}

void DownloadTask::fileDownloadFinished()
{
    emitSucceeded();
}

void DownloadTask::fileDownloadFailed(QString reason)
{
    qCritical() << "Failed to download update files:" << reason;
    emitFailed(tr("Failed to download update files: %1").arg(reason));
}

void DownloadTask::fileDownloadProgressChanged(qint64 current, qint64 total)
{
    setProgress(current, total);
}

QString DownloadTask::updateFilesDir()
{
    return m_updateFilesDir.path();
}

OperationList DownloadTask::operations()
{
    return m_operations;
}

}