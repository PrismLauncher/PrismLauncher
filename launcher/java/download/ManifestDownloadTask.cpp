// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2023-2024 Trial97 <alexandru.tripon97@gmail.com>
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
#include "java/download/ManifestDownloadTask.h"

#include "Application.h"
#include "FileSystem.h"
#include "Json.h"
#include "net/ChecksumValidator.h"
#include "net/NetJob.h"

struct File {
    QString path;
    QString url;
    QByteArray hash;
    bool isExec;
};

namespace Java {
ManifestDownloadTask::ManifestDownloadTask(QUrl url, QString final_path, QString checksumType, QString checksumHash)
    : m_url(url), m_final_path(final_path), m_checksum_type(checksumType), m_checksum_hash(checksumHash)
{}

void ManifestDownloadTask::executeTask()
{
    setStatus(tr("Downloading Java"));
    auto download = makeShared<NetJob>(QString("JRE::DownloadJava"), APPLICATION->network());
    auto files = std::make_shared<QByteArray>();

    auto action = Net::Download::makeByteArray(m_url, files);
    if (!m_checksum_hash.isEmpty() && !m_checksum_type.isEmpty()) {
        auto hashType = QCryptographicHash::Algorithm::Sha1;
        if (m_checksum_type == "sha256") {
            hashType = QCryptographicHash::Algorithm::Sha256;
        }
        action->addValidator(new Net::ChecksumValidator(hashType, QByteArray::fromHex(m_checksum_hash.toUtf8())));
    }
    download->addNetAction(action);

    connect(download.get(), &Task::failed, this, &ManifestDownloadTask::emitFailed);
    connect(download.get(), &Task::progress, this, &ManifestDownloadTask::setProgress);
    connect(download.get(), &Task::stepProgress, this, &ManifestDownloadTask::propagateStepProgress);
    connect(download.get(), &Task::status, this, &ManifestDownloadTask::setStatus);
    connect(download.get(), &Task::details, this, &ManifestDownloadTask::setDetails);

    connect(download.get(), &Task::succeeded, [files, this] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*files, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response at " << parse_error.offset << ". Reason: " << parse_error.errorString();
            qWarning() << *files;
            emitFailed(parse_error.errorString());
            return;
        }
        downloadJava(doc);
    });
    m_task = download;
    m_task->start();
}

void ManifestDownloadTask::downloadJava(const QJsonDocument& doc)
{
    // valid json doc, begin making jre spot
    FS::ensureFolderPathExists(m_final_path);
    std::vector<File> toDownload;
    auto list = Json::ensureObject(Json::ensureObject(doc.object()), "files");
    for (const auto& paths : list.keys()) {
        auto file = FS::PathCombine(m_final_path, paths);

        const QJsonObject& meta = Json::ensureObject(list, paths);
        auto type = Json::ensureString(meta, "type");
        if (type == "directory") {
            FS::ensureFolderPathExists(file);
        } else if (type == "link") {
            // this is linux only !
            auto path = Json::ensureString(meta, "target");
            if (!path.isEmpty()) {
                auto target = FS::PathCombine(file, "../" + path);
                QFile(target).link(file);
            }
        } else if (type == "file") {
            // TODO download compressed version if it exists ?
            auto raw = Json::ensureObject(Json::ensureObject(meta, "downloads"), "raw");
            auto isExec = Json::ensureBoolean(meta, "executable", false);
            auto url = Json::ensureString(raw, "url");
            if (!url.isEmpty() && QUrl(url).isValid()) {
                auto f = File{ file, url, QByteArray::fromHex(Json::ensureString(raw, "sha1").toLatin1()), isExec };
                toDownload.push_back(f);
            }
        }
    }
    auto elementDownload = makeShared<NetJob>("JRE::FileDownload", APPLICATION->network());
    for (const auto& file : toDownload) {
        auto dl = Net::Download::makeFile(file.url, file.path);
        if (!file.hash.isEmpty()) {
            dl->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha1, file.hash));
        }
        if (file.isExec) {
            connect(dl.get(), &Net::Download::succeeded,
                    [file] { QFile(file.path).setPermissions(QFile(file.path).permissions() | QFileDevice::Permissions(0x1111)); });
        }
        elementDownload->addNetAction(dl);
    }

    connect(elementDownload.get(), &Task::failed, this, &ManifestDownloadTask::emitFailed);
    connect(elementDownload.get(), &Task::progress, this, &ManifestDownloadTask::setProgress);
    connect(elementDownload.get(), &Task::stepProgress, this, &ManifestDownloadTask::propagateStepProgress);
    connect(elementDownload.get(), &Task::status, this, &ManifestDownloadTask::setStatus);
    connect(elementDownload.get(), &Task::details, this, &ManifestDownloadTask::setDetails);

    connect(elementDownload.get(), &Task::succeeded, this, &ManifestDownloadTask::emitSucceeded);
    m_task = elementDownload;
    m_task->start();
}

bool ManifestDownloadTask::abort()
{
    auto aborted = canAbort();
    if (m_task)
        aborted = m_task->abort();
    emitAborted();
    return aborted;
};
}  // namespace Java