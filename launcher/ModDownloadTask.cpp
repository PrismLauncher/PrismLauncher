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

#include "ModDownloadTask.h"
#include "Application.h"

ModDownloadTask::ModDownloadTask(const QUrl sourceUrl,const QString filename, const std::shared_ptr<ModFolderModel> mods)
: m_sourceUrl(sourceUrl), mods(mods), filename(filename) {
}

void ModDownloadTask::executeTask() {
    setStatus(tr("Downloading mod:\n%1").arg(m_sourceUrl.toString()));

    m_filesNetJob.reset(new NetJob(tr("Modpack download"), APPLICATION->network()));
    m_filesNetJob->addNetAction(Net::Download::makeFile(m_sourceUrl, mods->dir().absoluteFilePath(filename)));
    connect(m_filesNetJob.get(), &NetJob::succeeded, this, &ModDownloadTask::downloadSucceeded);
    connect(m_filesNetJob.get(), &NetJob::progress, this, &ModDownloadTask::downloadProgressChanged);
    connect(m_filesNetJob.get(), &NetJob::failed, this, &ModDownloadTask::downloadFailed);
    m_filesNetJob->start();
}

void ModDownloadTask::downloadSucceeded()
{
    emitSucceeded();
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

bool ModDownloadTask::abort() {
    return m_filesNetJob->abort();
}

