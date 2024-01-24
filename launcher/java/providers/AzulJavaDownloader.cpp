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
#include "java/providers/AzulJavaDownloader.h"
#include <qcontainerfwd.h>
#include "MMCZip.h"

#include "Application.h"
#include "Json.h"
#include "net/NetJob.h"
#include "tasks/Task.h"

void AzulJavaDownloader::executeTask()
{
    downloadJavaList();
};

void AzulJavaDownloader::downloadJavaList()
{
    setStatus(tr("Querying Azul meta"));

    auto javaVersion = m_is_legacy ? QString("8.0") : QString("17.0");
    auto azulOS = m_os_name == "osx" ? "macos" : m_os_name;
    auto arch = getArch();
    auto metaResponse = std::make_shared<QByteArray>();
    auto downloadJob = makeShared<NetJob>(QString("JRE::QueryAzulMeta"), APPLICATION->network());
    downloadJob->addNetAction(Net::Download::makeByteArray(QString("https://api.azul.com/metadata/v1/zulu/packages/?"
                                                                   "java_version=%1"
                                                                   "&os=%2"
                                                                   "&arch=%3"
                                                                   "&archive_type=zip"
                                                                   "&java_package_type=jre"
                                                                   "&support_term=lts"
                                                                   "&latest=true"
                                                                   "status=ga"
                                                                   "&availability_types=CA"
                                                                   "&page=1"
                                                                   "&page_size=1")
                                                               .arg(javaVersion, azulOS, arch),
                                                           metaResponse));
    connect(downloadJob.get(), &NetJob::finished,
            [downloadJob, metaResponse, this] { disconnect(this, &Task::aborted, downloadJob.get(), &NetJob::abort); });
    connect(this, &Task::aborted, downloadJob.get(), &NetJob::abort);
    connect(downloadJob.get(), &NetJob::failed, this, &AzulJavaDownloader::emitFailed);
    connect(downloadJob.get(), &NetJob::progress, this, &AzulJavaDownloader::progress);
    connect(downloadJob.get(), &NetJob::succeeded, [metaResponse, this] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*metaResponse, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response at " << parse_error.offset << " reason: " << parse_error.errorString();
            qWarning() << *metaResponse;
            return;
        }
        auto array = Json::ensureArray(doc.array());
        if (!array.empty()) {
            downloadJava(array);
        } else {
            emitFailed(tr("No suitable JRE found"));
        }
    });
    downloadJob->start();
};

QString AzulJavaDownloader::getArch() const
{
    if (m_os_arch == "arm64")
        return "aarch64";
    if (m_os_arch == "arm")
        return "aarch32";
    if (m_os_arch.isEmpty())
        return "x86";
    return m_os_arch;
}

void AzulJavaDownloader::downloadJava(const QJsonArray& array)
{
    // JRE found ! download the zip
    setStatus(tr("Downloading Java from Azul"));

    MetaEntryPtr entry = APPLICATION->metacache()->resolveEntry("java", "azulJRE.zip");

    auto downloadURL = QUrl(array[0].toObject()["url"].toString());
    auto download = makeShared<NetJob>(QString("JRE::DownloadJava"), APPLICATION->network());
    download->addNetAction(Net::Download::makeCached(downloadURL, entry));
    auto fullPath = entry->getFullPath();

    connect(download.get(), &NetJob::finished, [download, this] { disconnect(this, &Task::aborted, download.get(), &NetJob::abort); });
    // connect(download.get(), &NetJob::aborted, [path] { APPLICATION->instances()->destroyStagingPath(path); });
    connect(download.get(), &NetJob::progress, this, &AzulJavaDownloader::progress);
    connect(download.get(), &NetJob::failed, this, &AzulJavaDownloader::emitFailed);
    connect(this, &Task::aborted, download.get(), &NetJob::abort);
    connect(download.get(), &NetJob::succeeded, [downloadURL, this, fullPath] {
        // This should do all of the extracting and creating folders
        extractJava(fullPath, downloadURL.fileName().chopped(4));
    });
    download->start();
};

void AzulJavaDownloader::extractJava(QString input, QString subdirectory)
{
    setStatus(tr("Extracting java"));
    auto zipTask = makeShared<MMCZip::ExtractZipTask>(input, m_final_path, subdirectory);

    auto progressStep = std::make_shared<TaskStepProgress>();
    connect(zipTask.get(), &Task::finished, this, [this, progressStep] {
        progressStep->state = TaskStepState::Succeeded;
        stepProgress(*progressStep);
    });

    connect(this, &Task::aborted, zipTask.get(), &Task::abort);
    connect(zipTask.get(), &Task::finished, [zipTask, this] { disconnect(this, &Task::aborted, zipTask.get(), &Task::abort); });

    connect(zipTask.get(), &Task::succeeded, this, &AzulJavaDownloader::emitSucceeded);
    connect(zipTask.get(), &Task::aborted, this, &AzulJavaDownloader::emitAborted);
    connect(zipTask.get(), &Task::failed, this, [this, progressStep](QString reason) {
        progressStep->state = TaskStepState::Failed;
        stepProgress(*progressStep);
        emitFailed(reason);
    });
    connect(zipTask.get(), &Task::stepProgress, this, &AzulJavaDownloader::propagateStepProgress);

    connect(zipTask.get(), &Task::progress, this, [this, progressStep](qint64 current, qint64 total) {
        progressStep->update(current, total);
        stepProgress(*progressStep);
    });
    connect(zipTask.get(), &Task::status, this, [this, progressStep](QString status) {
        progressStep->status = status;
        stepProgress(*progressStep);
    });
    zipTask->start();
};

static const QStringList supportedOs = {
    "macos", "linux", "windows", "linux-musl", "linux-glibc", "qnx", "solaris", "aix",
};

static const QStringList supportedArch = {
    "x86",   "x64",   "amd64",   "i686",     "arm",   "aarch64", "aarch32", "aarch32sf", "aarch32hf",  "ppc",
    "ppc64", "ppc32", "ppc32hf", "ppc32spe", "sparc", "sparc64", "sparc32", "sparcv9",   "sparcv9-64", "sparcv9-32",
};

bool AzulJavaDownloader::isSupported() const
{
    return supportedOs.contains(m_os_name == "osx" ? "macos" : m_os_name) && supportedArch.contains(getArch());
};