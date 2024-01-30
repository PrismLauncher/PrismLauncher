// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2023 Trial97 <alexandru.tripon97@gmail.com>
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
#include "java/download/ArchiveJavaDownloader.h"
#include <quazip.h>
#include <memory>
#include "MMCZip.h"

#include "Application.h"
#include "net/ChecksumValidator.h"
#include "net/NetJob.h"
#include "tasks/Task.h"

ArchiveJavaDownloader::ArchiveJavaDownloader(QUrl url, QString final_path, QString checksumType, QString checksumHash)
    : m_url(url), m_final_path(final_path), m_checksum_type(checksumType), m_checksum_hash(checksumHash)
{}

void ArchiveJavaDownloader::executeTask()
{
    // JRE found ! download the zip
    setStatus(tr("Downloading Java"));

    MetaEntryPtr entry = APPLICATION->metacache()->resolveEntry("java", m_url.toLocalFile());

    auto download = makeShared<NetJob>(QString("JRE::DownloadJava"), APPLICATION->network());
    auto action = Net::Download::makeCached(m_url, entry);
    if (!m_checksum_hash.isEmpty() && !m_checksum_type.isEmpty()) {
        auto hashType = QCryptographicHash::Algorithm::Sha1;
        if (m_checksum_type == "sha256") {
            hashType = QCryptographicHash::Algorithm::Sha256;
        }
        action->addValidator(new Net::ChecksumValidator(hashType, QByteArray::fromHex(m_checksum_hash.toUtf8())));
    }
    download->addNetAction(action);
    auto fullPath = entry->getFullPath();

    connect(download.get(), &NetJob::finished, [download, this] { disconnect(this, &Task::aborted, download.get(), &NetJob::abort); });
    connect(download.get(), &NetJob::progress, this, &ArchiveJavaDownloader::progress);
    connect(download.get(), &NetJob::failed, this, &ArchiveJavaDownloader::emitFailed);
    connect(this, &Task::aborted, download.get(), &NetJob::abort);
    connect(download.get(), &NetJob::succeeded, [this, fullPath] {
        // This should do all of the extracting and creating folders
        extractJava(fullPath);
    });
    download->start();
}

void ArchiveJavaDownloader::extractJava(QString input)
{
    setStatus(tr("Extracting java"));
    auto zip = std::make_shared<QuaZip>(input);
    auto files = zip->getFileNameList();
    if (files.isEmpty()) {
        emitFailed("Empty archive");
        return;
    }
    auto zipTask = makeShared<MMCZip::ExtractZipTask>(input, m_final_path, files[0]);

    auto progressStep = std::make_shared<TaskStepProgress>();
    connect(zipTask.get(), &Task::finished, this, [this, progressStep] {
        progressStep->state = TaskStepState::Succeeded;
        stepProgress(*progressStep);
    });

    connect(this, &Task::aborted, zipTask.get(), &Task::abort);
    connect(zipTask.get(), &Task::finished, [zipTask, this] { disconnect(this, &Task::aborted, zipTask.get(), &Task::abort); });

    connect(zipTask.get(), &Task::succeeded, this, &ArchiveJavaDownloader::emitSucceeded);
    connect(zipTask.get(), &Task::aborted, this, &ArchiveJavaDownloader::emitAborted);
    connect(zipTask.get(), &Task::failed, this, [this, progressStep](QString reason) {
        progressStep->state = TaskStepState::Failed;
        stepProgress(*progressStep);
        emitFailed(reason);
    });
    connect(zipTask.get(), &Task::stepProgress, this, &ArchiveJavaDownloader::propagateStepProgress);

    connect(zipTask.get(), &Task::progress, this, [this, progressStep](qint64 current, qint64 total) {
        progressStep->update(current, total);
        stepProgress(*progressStep);
    });
    connect(zipTask.get(), &Task::status, this, [this, progressStep](QString status) {
        progressStep->status = status;
        stepProgress(*progressStep);
    });
    zipTask->start();
}