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
#include "java/providers/MojanglJavaDownloader.h"

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

void MojangJavaDownloader::executeTask()
{
    downloadJavaList();
};

void MojangJavaDownloader::downloadJavaList()
{
    auto netJob = makeShared<NetJob>(QString("JRE::QueryVersions"), APPLICATION->network());
    auto response = std::make_shared<QByteArray>();
    setStatus(tr("Querying mojang meta"));
    netJob->addNetAction(Net::Download::makeByteArray(
        QUrl("https://piston-meta.mojang.com/v1/products/java-runtime/2ec0cc96c44e5a76b9c8b7c39df7210883d12871/all.json"), response));

    connect(netJob.get(), &NetJob::finished, [netJob, this] {
        // delete so that it's not called on a deleted job
        // FIXME: is this needed? qt should handle this
        disconnect(this, &Task::aborted, netJob.get(), &NetJob::abort);
    });
    connect(this, &Task::aborted, netJob.get(), &NetJob::abort);

    connect(netJob.get(), &NetJob::progress, this, &MojangJavaDownloader::progress);
    connect(netJob.get(), &NetJob::failed, this, &MojangJavaDownloader::emitFailed);
    connect(netJob.get(), &NetJob::succeeded, [response, this, netJob] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response at " << parse_error.offset << " reason: " << parse_error.errorString();
            qWarning() << *response;
            emitFailed(parse_error.errorString());
            return;
        }
        auto versionArray = Json::ensureArray(Json::ensureObject(doc.object(), getOS()), m_is_legacy ? "jre-legacy" : "java-runtime-gamma");
        if (!versionArray.empty()) {
            parseManifest(versionArray);

        } else {
            // mojang does not have a JRE for us, so fail
            emitFailed("No suitable JRE found");
        }
    });

    netJob->start();
};

QString MojangJavaDownloader::getOS() const
{
    if (m_os_name == "windows") {
        if (m_os_arch == "x86_64") {
            return "windows-x64";
        }
        if (m_os_arch == "i386") {
            return "windows-x86";
        }
        // Unknown, maybe arm, appending arch for downloader
        return "windows-" + m_os_arch;
    }
    if (m_os_name == "osx") {
        if (m_os_arch == "arm64") {
            return "mac-os-arm64";
        }
        return "mac-os";
    }
    if (m_os_name == "linux") {
        if (m_os_arch == "x86_64") {
            return "linux";
        }
        // will work for i386, and arm(64)
        return "linux-" + m_os_arch;
    }
    return {};
}
void MojangJavaDownloader::parseManifest(const QJsonArray& versionArray)
{
    setStatus(tr("Downloading Java from Mojang"));
    auto url = Json::ensureString(Json::ensureObject(Json::ensureObject(versionArray[0]), "manifest"), "url");
    auto download = makeShared<NetJob>(QString("JRE::DownloadJava"), APPLICATION->network());
    auto files = std::make_shared<QByteArray>();

    download->addNetAction(Net::Download::makeByteArray(QUrl(url), files));

    connect(download.get(), &NetJob::finished, [download, this] { disconnect(this, &Task::aborted, download.get(), &NetJob::abort); });
    connect(download.get(), &NetJob::progress, this, &MojangJavaDownloader::progress);
    connect(download.get(), &NetJob::failed, this, &MojangJavaDownloader::emitFailed);
    connect(this, &Task::aborted, download.get(), &NetJob::abort);

    connect(download.get(), &NetJob::succeeded, [files, this] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*files, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response at " << parse_error.offset << " reason: " << parse_error.errorString();
            qWarning() << *files;
            emitFailed(parse_error.errorString());
            return;
        }
        downloadJava(doc);
    });
    download->start();
};

void MojangJavaDownloader::downloadJava(const QJsonDocument& doc)
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
    auto elementDownload = new NetJob("JRE::FileDownload", APPLICATION->network());
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
    connect(elementDownload, &NetJob::finished, [elementDownload, this] {
        disconnect(this, &Task::aborted, elementDownload, &NetJob::abort);
        elementDownload->deleteLater();
    });
    connect(elementDownload, &NetJob::progress, this, &MojangJavaDownloader::progress);
    connect(elementDownload, &NetJob::failed, this, &MojangJavaDownloader::emitFailed);

    connect(this, &Task::aborted, elementDownload, &NetJob::abort);
    connect(elementDownload, &NetJob::succeeded, [this] { emitSucceeded(); });
    elementDownload->start();
};
