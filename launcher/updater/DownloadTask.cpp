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

#include "DownloadTask.h"

#include "updater/UpdateChecker.h"
#include "GoUpdate.h"
#include "net/NetJob.h"

#include <QFile>
#include <QTemporaryDir>
#include <QCryptographicHash>

namespace GoUpdate
{

DownloadTask::DownloadTask(
    shared_qobject_ptr<QNetworkAccessManager> network,
    Status status,
    QString target,
    QObject *parent
) : Task(parent), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateFilesDir(target), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_network(network)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_status = status;

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateFilesDir.setAutoRemove(false);
}

void DownloadTask::executeTask()
{
    loadVersionInfo();
}

void DownloadTask::loadVersionInfo()
{
    setStatus(tr("Loading version information..."));

    NetJob *netJob = new NetJob("Version Info", hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_network);

    // Find the index URL.
    QUrl newIndexUrl = QUrl(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_status.newRepoUrl).resolved(QString::number(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_status.newVersionId) + ".json");
    qDebug() << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_status.newRepoUrl << " turns into " << newIndexUrl;

    netJob->addNetAction(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_newVersionFileListDownload = Net::Download::makeByteArray(newIndexUrl, &newVersionFileListData));

    // If we have a current version URL, get that one too.
    if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_status.currentRepoUrl.isEmpty())
    {
        QUrl cIndexUrl = QUrl(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_status.currentRepoUrl).resolved(QString::number(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_status.currentVersionId) + ".json");
        netJob->addNetAction(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentVersionFileListDownload = Net::Download::makeByteArray(cIndexUrl, &currentVersionFileListData));
        qDebug() << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_status.currentRepoUrl << " turns into " << cIndexUrl;
    }

    // connect signals and start the job
    connect(netJob, &NetJob::succeeded, this, &DownloadTask::processDownloadedVersionInfo);
    connect(netJob, &NetJob::failed, this, &DownloadTask::vinfoDownloadFailed);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_vinfoNetJob.reset(netJob);
    netJob->start();
}

void DownloadTask::vinfoDownloadFailed()
{
    // Something failed. We really need the second download (current version info), so parse
    // downloads anyways as long as the first one succeeded.
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_newVersionFileListDownload->wasSuccessful())
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
    VersionFileList hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentVersionFileList;
    VersionFileList hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_newVersionFileList;

    setStatus(tr("Reading file list for new version..."));
    qDebug() << "Reading file list for new version...";
    QString error;
    if (!parseVersionInfo(newVersionFileListData, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_newVersionFileList, error))
    {
        qCritical() << error;
        emitFailed(error);
        return;
    }

    // if we have the current version info, use it.
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentVersionFileListDownload && hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentVersionFileListDownload->wasSuccessful())
    {
        setStatus(tr("Reading file list for current version..."));
        qDebug() << "Reading file list for current version...";
        // if this fails, it's not a complete loss.
        QString error;
        if(!parseVersionInfo( currentVersionFileListData, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentVersionFileList, error))
        {
            qDebug() << error << "This is not a fatal error.";
        }
    }

    // We don't need this any more.
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentVersionFileListDownload.reset();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_newVersionFileListDownload.reset();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_vinfoNetJob.reset();

    setStatus(tr("Processing file lists - figuring out how to install the update..."));

    // make a new netjob for the actual update files
    NetJob::Ptr netJob = new NetJob("Update Files", hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_network);

    // fill netJob and operationList
    if (!processFileLists(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentVersionFileList, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_newVersionFileList, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_status.rootPath, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateFilesDir.path(), netJob, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_operations))
    {
        emitFailed(tr("Failed to process update lists..."));
        return;
    }

    // Now start the download.
    QObject::connect(netJob.get(), &NetJob::succeeded, this, &DownloadTask::fileDownloadFinished);
    QObject::connect(netJob.get(), &NetJob::progress, this, &DownloadTask::fileDownloadProgressChanged);
    QObject::connect(netJob.get(), &NetJob::failed, this, &DownloadTask::fileDownloadFailed);

    if(netJob->size() == 1) // Translation issues... see https://github.com/MultiMC/Launcher/issues/1701
    {
        setStatus(tr("Downloading one update file."));
    }
    else
    {
        setStatus(tr("Downloading %1 update files.").arg(QString::number(netJob->size())));
    }
    qDebug() << "Begin downloading update files to" << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateFilesDir.path();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob = netJob;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filesNetJob->start();
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
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateFilesDir.path();
}

OperationList DownloadTask::operations()
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_operations;
}

}
