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
#include "java/providers/AdoptiumJavaDownloader.h"
#include <quazip.h>
#include <memory>
#include "MMCZip.h"

#include "Application.h"
#include "net/NetJob.h"
#include "tasks/Task.h"

void AdoptiumJavaDownloader::executeTask()
{
    downloadJava();
};

QString AdoptiumJavaDownloader::getArch() const
{
    if (m_os_arch == "arm64")
        return "aarch64";
    if (m_os_arch.isEmpty())
        return "x86";
    return m_os_arch;
}

void AdoptiumJavaDownloader::downloadJava()
{
    // JRE found ! download the zip
    setStatus(tr("Downloading Java from Adoptium"));

    auto javaVersion = m_is_legacy ? QString("8") : QString("17");
    auto azulOS = m_os_name == "osx" ? "mac" : m_os_name;
    auto arch = getArch();
    MetaEntryPtr entry = APPLICATION->metacache()->resolveEntry("java", "adoptiumJRE.zip");

    auto download = makeShared<NetJob>(QString("JRE::DownloadJava"), APPLICATION->network());
    download->addNetAction(Net::Download::makeCached(
        QString("https://api.adoptium.net/v3/binary/latest/%1/ga/%2/%3/jre/hotspot/normal/eclipse").arg(javaVersion, azulOS, arch), entry));
    auto fullPath = entry->getFullPath();

    connect(download.get(), &NetJob::finished, [download, this] { disconnect(this, &Task::aborted, download.get(), &NetJob::abort); });
    // connect(download.get(), &NetJob::aborted, [path] { APPLICATION->instances()->destroyStagingPath(path); });
    connect(download.get(), &NetJob::progress, this, &AdoptiumJavaDownloader::progress);
    connect(download.get(), &NetJob::failed, this, &AdoptiumJavaDownloader::emitFailed);
    connect(this, &Task::aborted, download.get(), &NetJob::abort);
    connect(download.get(), &NetJob::succeeded, [this, fullPath] {
        // This should do all of the extracting and creating folders
        extractJava(fullPath);
    });
    download->start();
};

void AdoptiumJavaDownloader::extractJava(QString input)
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

    connect(zipTask.get(), &Task::succeeded, this, &AdoptiumJavaDownloader::emitSucceeded);
    connect(zipTask.get(), &Task::aborted, this, &AdoptiumJavaDownloader::emitAborted);
    connect(zipTask.get(), &Task::failed, this, [this, progressStep](QString reason) {
        progressStep->state = TaskStepState::Failed;
        stepProgress(*progressStep);
        emitFailed(reason);
    });
    connect(zipTask.get(), &Task::stepProgress, this, &AdoptiumJavaDownloader::propagateStepProgress);

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
    "linux", "windows", "mac", "solaris", "aix", "alpine-linux",
};

static const QStringList supportedArch = {
    "x64", "x86", "x32", "ppc64", "ppc64le", "s390x", "aarch64", "arm", "sparcv9", "riscv64",
};

bool AdoptiumJavaDownloader::isSupported() const
{
    return supportedOs.contains(m_os_name == "osx" ? "mac" : m_os_name) && supportedArch.contains(getArch());
};