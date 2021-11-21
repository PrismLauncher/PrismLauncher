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

#include "SolderPackInstallTask.h"

#include <FileSystem.h>
#include <Json.h>
#include <QtConcurrentRun>
#include <MMCZip.h>
#include "TechnicPackProcessor.h"

Technic::SolderPackInstallTask::SolderPackInstallTask(
    shared_qobject_ptr<QNetworkAccessManager> network,
    const QUrl &sourceUrl,
    const QString &minecraftVersion
) {
    m_sourceUrl = sourceUrl;
    m_minecraftVersion = minecraftVersion;
    m_network = network;
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
    setStatus(tr("Finding recommended version:\n%1").arg(m_sourceUrl.toString()));
    m_filesNetJob.reset(new NetJob(tr("Finding recommended version")));
    m_filesNetJob->addNetAction(Net::Download::makeByteArray(m_sourceUrl, &m_response));
    auto job = m_filesNetJob.get();
    connect(job, &NetJob::succeeded, this, &Technic::SolderPackInstallTask::versionSucceeded);
    connect(job, &NetJob::failed, this, &Technic::SolderPackInstallTask::downloadFailed);
    m_filesNetJob->start(m_network);
}

void Technic::SolderPackInstallTask::versionSucceeded()
{
    try
    {
        QJsonDocument doc = Json::requireDocument(m_response);
        QJsonObject obj = Json::requireObject(doc);
        QString version = Json::requireString(obj, "recommended", "__placeholder__");
        m_sourceUrl = m_sourceUrl.toString() + '/' + version;
    }
    catch (const JSONValidationError &e)
    {
        emitFailed(e.cause());
        m_filesNetJob.reset();
        return;
    }

    setStatus(tr("Resolving modpack files:\n%1").arg(m_sourceUrl.toString()));
    m_filesNetJob.reset(new NetJob(tr("Resolving modpack files")));
    m_filesNetJob->addNetAction(Net::Download::makeByteArray(m_sourceUrl, &m_response));
    auto job = m_filesNetJob.get();
    connect(job, &NetJob::succeeded, this, &Technic::SolderPackInstallTask::fileListSucceeded);
    connect(job, &NetJob::failed, this, &Technic::SolderPackInstallTask::downloadFailed);
    m_filesNetJob->start(m_network);
}

void Technic::SolderPackInstallTask::fileListSucceeded()
{
    setStatus(tr("Downloading modpack:"));
    QStringList modUrls;
    try
    {
        QJsonDocument doc = Json::requireDocument(m_response);
        QJsonObject obj = Json::requireObject(doc);
        QString minecraftVersion = Json::ensureString(obj, "minecraft", QString(), "__placeholder__");
        if (!minecraftVersion.isEmpty())
            m_minecraftVersion = minecraftVersion;
        QJsonArray mods = Json::requireArray(obj, "mods", "'mods'");
        for (auto mod: mods)
        {
            QJsonObject modObject = Json::requireObject(mod);
            modUrls.append(Json::requireString(modObject, "url", "'url'"));
        }
    }
    catch (const JSONValidationError &e)
    {
        emitFailed(e.cause());
        m_filesNetJob.reset();
        return;
    }
    m_filesNetJob.reset(new NetJob(tr("Downloading modpack")));
    int i = 0;
    for (auto &modUrl: modUrls)
    {
        auto path = FS::PathCombine(m_outputDir.path(), QString("%1").arg(i));
        m_filesNetJob->addNetAction(Net::Download::makeFile(modUrl, path));
        i++;
    }

    m_modCount = modUrls.size();

    connect(m_filesNetJob.get(), &NetJob::succeeded, this, &Technic::SolderPackInstallTask::downloadSucceeded);
    connect(m_filesNetJob.get(), &NetJob::progress, this, &Technic::SolderPackInstallTask::downloadProgressChanged);
    connect(m_filesNetJob.get(), &NetJob::failed, this, &Technic::SolderPackInstallTask::downloadFailed);
    m_filesNetJob->start(m_network);
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

    shared_qobject_ptr<Technic::TechnicPackProcessor> packProcessor = new Technic::TechnicPackProcessor();
    connect(packProcessor.get(), &Technic::TechnicPackProcessor::succeeded, this, &Technic::SolderPackInstallTask::emitSucceeded);
    connect(packProcessor.get(), &Technic::TechnicPackProcessor::failed, this, &Technic::SolderPackInstallTask::emitFailed);
    packProcessor->run(m_globalSettings, m_instName, m_instIcon, m_stagingPath, m_minecraftVersion, true);
}

void Technic::SolderPackInstallTask::extractAborted()
{
    emitFailed(tr("Instance import has been aborted."));
    return;
}

