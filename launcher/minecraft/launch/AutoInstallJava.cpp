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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "AutoInstallJava.h"
#include <QDir>
#include <QFileInfo>
#include <memory>

#include "Application.h"
#include "FileSystem.h"
#include "MessageLevel.h"
#include "QObjectPtr.h"
#include "SysInfo.h"
#include "java/JavaInstall.h"
#include "java/JavaInstallList.h"
#include "java/JavaUtils.h"
#include "java/JavaVersion.h"
#include "java/download/ArchiveDownloadTask.h"
#include "java/download/ManifestDownloadTask.h"
#include "java/download/SymlinkTask.h"
#include "meta/Index.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "net/Mode.h"
#include "tasks/SequentialTask.h"

AutoInstallJava::AutoInstallJava(LaunchTask* parent)
    : LaunchStep(parent), m_instance(m_parent->instance()), m_supported_arch(SysInfo::getSupportedJavaArchitecture()) {};

void AutoInstallJava::executeTask()
{
    auto settings = m_instance->settings();
    if (!APPLICATION->settings()->get("AutomaticJavaSwitch").toBool() ||
        (settings->get("OverrideJavaLocation").toBool() && QFileInfo::exists(settings->get("JavaPath").toString()))) {
        emitSucceeded();
        return;
    }
    auto packProfile = m_instance->getPackProfile();
    if (!APPLICATION->settings()->get("AutomaticJavaDownload").toBool()) {
        auto javas = APPLICATION->javalist();
        m_current_task = javas->getLoadTask();
        connect(m_current_task.get(), &Task::finished, this, [this, javas, packProfile] {
            for (auto i = 0; i < javas->count(); i++) {
                auto java = std::dynamic_pointer_cast<JavaInstall>(javas->at(i));
                if (java && packProfile->getProfile()->getCompatibleJavaMajors().contains(java->id.major())) {
                    if (!java->is_64bit) {
                        emit logLine(tr("The automatic Java mechanism detected a 32-bit installation of Java."), MessageLevel::Launcher);
                    }
                    setJavaPath(java->path);
                    return;
                }
            }
            emit logLine(tr("No compatible Java version was found. Using the default one."), MessageLevel::Warning);
            emitSucceeded();
        });
        connect(m_current_task.get(), &Task::progress, this, &AutoInstallJava::setProgress);
        connect(m_current_task.get(), &Task::stepProgress, this, &AutoInstallJava::propagateStepProgress);
        connect(m_current_task.get(), &Task::status, this, &AutoInstallJava::setStatus);
        connect(m_current_task.get(), &Task::details, this, &AutoInstallJava::setDetails);
        emit progressReportingRequest();
        return;
    }
    if (m_supported_arch.isEmpty()) {
        emit logLine(tr("Your system (%1-%2) is not compatible with automatic Java installation. Using the default Java path.")
                         .arg(SysInfo::currentSystem(), SysInfo::useQTForArch()),
                     MessageLevel::Warning);
        emitSucceeded();
        return;
    }
    auto wantedJavaName = packProfile->getProfile()->getCompatibleJavaName();
    if (wantedJavaName.isEmpty()) {
        emit logLine(tr("Your meta information is out of date or doesn't have the information necessary to determine what installation of "
                        "Java should be used. "
                        "Using the default Java path."),
                     MessageLevel::Warning);
        emitSucceeded();
        return;
    }
    QDir javaDir(APPLICATION->javaPath());
    auto wantedJavaPath = javaDir.absoluteFilePath(wantedJavaName);
    if (QFileInfo::exists(wantedJavaPath)) {
        setJavaPathFromPartial();
        return;
    }
    auto versionList = APPLICATION->metadataIndex()->get("net.minecraft.java");
    m_current_task = versionList->getLoadTask();
    connect(m_current_task.get(), &Task::succeeded, this, &AutoInstallJava::tryNextMajorJava);
    connect(m_current_task.get(), &Task::failed, this, &AutoInstallJava::emitFailed);
    connect(m_current_task.get(), &Task::progress, this, &AutoInstallJava::setProgress);
    connect(m_current_task.get(), &Task::stepProgress, this, &AutoInstallJava::propagateStepProgress);
    connect(m_current_task.get(), &Task::status, this, &AutoInstallJava::setStatus);
    connect(m_current_task.get(), &Task::details, this, &AutoInstallJava::setDetails);
    if (!m_current_task->isRunning()) {
        m_current_task->start();
    }
    emit progressReportingRequest();
}

void AutoInstallJava::setJavaPath(QString path)
{
    auto settings = m_instance->settings();
    settings->set("OverrideJavaLocation", true);
    settings->set("JavaPath", path);
    settings->set("AutomaticJava", true);
    emit logLine(tr("Compatible Java found at: %1.").arg(path), MessageLevel::Launcher);
    emitSucceeded();
}

void AutoInstallJava::setJavaPathFromPartial()
{
    auto packProfile = m_instance->getPackProfile();
    auto javaName = packProfile->getProfile()->getCompatibleJavaName();
    QDir javaDir(APPLICATION->javaPath());
    // just checking if the executable is there should suffice
    // but if needed this can be achieved through refreshing the javalist
    // and retrieving the path that contains the java name
    auto relativeBinary = FS::PathCombine(javaName, "bin", JavaUtils::javaExecutable);
    auto finalPath = javaDir.absoluteFilePath(relativeBinary);
    if (QFileInfo::exists(finalPath)) {
        setJavaPath(finalPath);
    } else {
        emit logLine(tr("No compatible Java version was found (the binary file does not exist). Using the default one."),
                     MessageLevel::Warning);
        emitSucceeded();
    }
    return;
}

void AutoInstallJava::downloadJava(Meta::Version::Ptr version, QString javaName)
{
    auto runtimes = version->data()->runtimes;
    for (auto java : runtimes) {
        if (java->runtimeOS == m_supported_arch && java->name() == javaName) {
            QDir javaDir(APPLICATION->javaPath());
            auto final_path = javaDir.absoluteFilePath(java->m_name);
            switch (java->downloadType) {
                case Java::DownloadType::Manifest:
                    m_current_task = makeShared<Java::ManifestDownloadTask>(java->url, final_path, java->checksumType, java->checksumHash);
                    break;
                case Java::DownloadType::Archive:
                    m_current_task = makeShared<Java::ArchiveDownloadTask>(java->url, final_path, java->checksumType, java->checksumHash);
                    break;
                case Java::DownloadType::Unknown:
                    emitFailed(tr("Could not determine Java download type!"));
                    return;
            }
#if defined(Q_OS_MACOS)
            auto seq = makeShared<SequentialTask>(tr("Install Java"));
            seq->addTask(m_current_task);
            seq->addTask(makeShared<Java::SymlinkTask>(final_path));
            m_current_task = seq;
#endif
            auto deletePath = [final_path] { FS::deletePath(final_path); };
            connect(m_current_task.get(), &Task::failed, this, [this, deletePath](QString reason) {
                deletePath();
                emitFailed(reason);
            });
            connect(m_current_task.get(), &Task::aborted, this, [deletePath] { deletePath(); });
            connect(m_current_task.get(), &Task::succeeded, this, &AutoInstallJava::setJavaPathFromPartial);
            connect(m_current_task.get(), &Task::failed, this, &AutoInstallJava::tryNextMajorJava);
            connect(m_current_task.get(), &Task::progress, this, &AutoInstallJava::setProgress);
            connect(m_current_task.get(), &Task::stepProgress, this, &AutoInstallJava::propagateStepProgress);
            connect(m_current_task.get(), &Task::status, this, &AutoInstallJava::setStatus);
            connect(m_current_task.get(), &Task::details, this, &AutoInstallJava::setDetails);
            m_current_task->start();
            return;
        }
    }
    tryNextMajorJava();
}

void AutoInstallJava::tryNextMajorJava()
{
    if (!isRunning())
        return;
    auto versionList = APPLICATION->metadataIndex()->get("net.minecraft.java");
    auto packProfile = m_instance->getPackProfile();
    auto wantedJavaName = packProfile->getProfile()->getCompatibleJavaName();
    auto majorJavaVersions = packProfile->getProfile()->getCompatibleJavaMajors();
    if (m_majorJavaVersionIndex >= majorJavaVersions.length()) {
        emit logLine(
            tr("No versions of Java were found for your operating system: %1-%2").arg(SysInfo::currentSystem(), SysInfo::useQTForArch()),
            MessageLevel::Warning);
        emit logLine(tr("No compatible version of Java was found. Using the default one."), MessageLevel::Warning);
        emitSucceeded();
        return;
    }
    auto majorJavaVersion = majorJavaVersions[m_majorJavaVersionIndex];
    m_majorJavaVersionIndex++;

    auto javaMajor = versionList->getVersion(QString("java%1").arg(majorJavaVersion));

    if (javaMajor->isLoaded()) {
        downloadJava(javaMajor, wantedJavaName);
    } else {
        m_current_task = APPLICATION->metadataIndex()->loadVersion("net.minecraft.java", javaMajor->version(), Net::Mode::Online);
        connect(m_current_task.get(), &Task::succeeded, this,
                [this, javaMajor, wantedJavaName] { downloadJava(javaMajor, wantedJavaName); });
        connect(m_current_task.get(), &Task::failed, this, &AutoInstallJava::tryNextMajorJava);
        connect(m_current_task.get(), &Task::progress, this, &AutoInstallJava::setProgress);
        connect(m_current_task.get(), &Task::stepProgress, this, &AutoInstallJava::propagateStepProgress);
        connect(m_current_task.get(), &Task::status, this, &AutoInstallJava::setStatus);
        connect(m_current_task.get(), &Task::details, this, &AutoInstallJava::setDetails);
        if (!m_current_task->isRunning()) {
            m_current_task->start();
        }
    }
}
bool AutoInstallJava::abort()
{
    if (m_current_task && m_current_task->canAbort()) {
        auto status = m_current_task->abort();
        emitFailed("Aborted.");
        return status;
    }
    return Task::abort();
}
