// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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

#include "LauncherPartLaunch.h"

#include <QRegularExpression>
#include <QStandardPaths>

#include "Application.h"
#include "Commandline.h"
#include "ExpectedHelpers.h"
#include "FileSystem.h"
#include "launch/LaunchTask.h"
#include "minecraft/MinecraftInstance.h"

#ifdef Q_OS_WIN32
#include "windows/WindowsAppContainer.h"
#elifdef Q_OS_LINUX
#include "gamemode_client.h"
#endif

LauncherPartLaunch::LauncherPartLaunch(LaunchTask* parent)
    : LaunchStep(parent)
    , m_process(parent->instance()->getJavaVersion().defaultsToUtf8() ? QStringConverter::Utf8 : QStringConverter::System)
{
    if (parent->instance()->settings()->get("CloseAfterLaunch").toBool()) {
        static const QRegularExpression s_settingUser(".*Setting user.+", QRegularExpression::CaseInsensitiveOption);
        std::shared_ptr<QMetaObject::Connection> connection{ new QMetaObject::Connection };
        *connection =
            connect(&m_process, &LoggedProcess::log, this, [connection](const QStringList& lines, [[maybe_unused]] MessageLevel level) {
                qDebug() << lines;
                if (lines.filter(s_settingUser).length() != 0) {
                    APPLICATION->closeAllWindows();
                    disconnect(*connection);
                }
            });
    }

    connect(&m_process, &LoggedProcess::log, this, &LauncherPartLaunch::logLines);
    connect(&m_process, &LoggedProcess::stateChanged, this, &LauncherPartLaunch::on_state);
}

LauncherPartLaunch::~LauncherPartLaunch() = default;

void LauncherPartLaunch::executeTask()
{
    QString jarPath = APPLICATION->getJarPath("NewLaunch.jar");
    if (jarPath.isEmpty()) {
        const char* reason = QT_TR_NOOP("Launcher library could not be found. Please check your installation.");
        emit logLine(tr(reason), MessageLevel::Fatal);
        emitFailed(tr(reason));
        return;
    }

    auto instance = m_parent->instance();

    QString legacyJarPath;
    if (instance->getLauncher() == "legacy" || instance->shouldApplyOnlineFixes()) {
        legacyJarPath = APPLICATION->getJarPath("NewLaunchLegacy.jar");
        if (legacyJarPath.isEmpty()) {
            const char* reason = QT_TR_NOOP("Legacy launcher library could not be found. Please check your installation.");
            emit logLine(tr(reason), MessageLevel::Fatal);
            emitFailed(tr(reason));
            return;
        }
    }

    m_launchScript = instance->createLaunchScript(m_session, m_targetToJoin);
    QStringList args = instance->javaArguments();
    QString allArgs = args.join(" ");
    emit logLine("Java arguments:\n  " + m_parent->censorPrivateInfo(allArgs) + "\n", MessageLevel::Launcher);

    auto javaPath = FS::ResolveExecutable(instance->settings()->get("JavaPath").toString());

    m_process.setProcessEnvironment(instance->createLaunchEnvironment());

    // make detachable - this will keep the process running even if the object is destroyed
    m_process.setDetachable(true);

    auto classPath = instance->getClassPath();
    QStringList extraSandboxPaths;
    classPath.prepend(jarPath);
    extraSandboxPaths.prepend(jarPath);

    if (!legacyJarPath.isEmpty()) {
        classPath.prepend(legacyJarPath);
        extraSandboxPaths.prepend(legacyJarPath);
    }

    if (const auto result = setupSandbox(javaPath, extraSandboxPaths); !result) {
        constexpr auto reason = QT_TR_NOOP("Unable to set up sandbox: %1");
        const auto error = QString::fromStdString(result.error().message());

        emit logLine(tr(reason).arg(error), MessageLevel::Fatal);
        emitFailed(QString(reason).arg(error));
        return;
    }

    auto natPath = instance->getNativePath();
#ifdef Q_OS_WIN
    natPath = FS::getPathNameInLocal8bit(natPath);
#endif
    args << "-Djava.library.path=" + natPath;

    args << "-cp";
#ifdef Q_OS_WIN
    QStringList processed;
    for (auto& item : classPath) {
        processed << FS::getPathNameInLocal8bit(item);
    }
    args << processed.join(';');
#else
    args << classPath.join(':');
#endif
    args << "org.prismlauncher.EntryPoint";

    qDebug() << args.join(' ');

    QString wrapperCommandStr = instance->getWrapperCommand().trimmed();
    if (!wrapperCommandStr.isEmpty()) {
        wrapperCommandStr = m_parent->substituteVariables(wrapperCommandStr);
        auto wrapperArgs = Commandline::splitArgs(wrapperCommandStr);
        auto wrapperCommand = wrapperArgs.takeFirst();
        auto realWrapperCommand = QStandardPaths::findExecutable(wrapperCommand);
        if (realWrapperCommand.isEmpty()) {
            const char* reason = QT_TR_NOOP("The wrapper command \"%1\" couldn't be found.");
            emit logLine(QString(reason).arg(wrapperCommand), MessageLevel::Fatal);
            emitFailed(tr(reason).arg(wrapperCommand));
            return;
        }
        emit logLine("Wrapper command is:\n" + wrapperCommandStr + "\n\n", MessageLevel::Launcher);
        args.prepend(javaPath);
        m_process.start(wrapperCommand, wrapperArgs + args);
    } else {
        m_process.start(javaPath, args);
    }

#ifdef Q_OS_LINUX
    if (instance->settings()->get("EnableFeralGamemode").toBool() && APPLICATION->capabilities() & Application::SupportsGameMode) {
        auto pid = m_process.processId();
        if (pid) {
            gamemode_request_start_for(pid);
        }
    }
#endif
}

void LauncherPartLaunch::on_state(LoggedProcess::State state)
{
    switch (state) {
        case LoggedProcess::FailedToStart: {
            //: Error message displayed if instace can't start
            const char* reason = QT_TR_NOOP("Could not launch Minecraft: %1");
            emit logLine(QString(reason).arg(m_process.errorString()), MessageLevel::Fatal);
            emitFailed(tr(reason).arg(m_process.errorString()));
            return;
        }
        case LoggedProcess::Aborted:
        case LoggedProcess::Crashed: {
            m_parent->setPid(-1);
            m_parent->instance()->setMinecraftRunning(false);
            emitFailed(tr("Game crashed."));
            return;
        }
        case LoggedProcess::Finished: {
            auto instance = m_parent->instance();
            if (instance->settings()->get("CloseAfterLaunch").toBool())
                APPLICATION->showMainWindow();

            m_parent->setPid(-1);
            m_parent->instance()->setMinecraftRunning(false);
            // if the exit code wasn't 0, report this as a crash
            auto exitCode = m_process.exitCode();
            if (exitCode != 0) {
                emitFailed(tr("Game crashed."));
                return;
            }
            // FIXME: make this work again
            //  m_postlaunchprocess.processEnvironment().insert("INST_EXITCODE", QString(exitCode));
            //  run post-exit
            emitSucceeded();
            break;
        }
        case LoggedProcess::Running:
            emit logLine(QString("Minecraft process ID: %1\n\n").arg(m_process.processId()), MessageLevel::Launcher);
            m_parent->setPid(m_process.processId());
            // send the launch script to the launcher part
            m_process.write(m_launchScript.toUtf8());

            mayProceed = true;
            emit readyForLaunch();
            break;
        default:
            break;
    }
}

std::expected<void, std::error_code> LauncherPartLaunch::setupSandbox(const QString& javaPath, const QStringList& extraPaths)
{
#ifdef Q_OS_WIN32
    auto appContainerResult = WindowsAppContainer::create();
    if (!appContainerResult) {
        return UNEXPECTED_WIN32_ERROR(appContainerResult.error());
    }
    m_appContainer = std::move(appContainerResult.value());

    auto splitPath = javaPath.split('/'); // java/bin/javaw.exe
    splitPath.removeLast(); // java/bin
    splitPath.removeLast(); // java
    const auto javaDir = splitPath.join('/');
    TRY(m_appContainer->grantFSAccess(javaDir, WindowsAppContainer::AccessMode::Read));

    for (const QString& path : extraPaths) {
        TRY(m_appContainer->grantFSAccess(path, WindowsAppContainer::AccessMode::Read));
    }

    TRY(m_appContainer->grantFSAccess(m_parent->instance()->librariesPath().path(), WindowsAppContainer::AccessMode::ReadWrite));  // Write access needed for ForgeWrapper

    if (auto nativePath = m_parent->instance()->getNativePath(); QFileInfo::exists(nativePath)) {
        TRY(m_appContainer->grantFSAccess(nativePath, WindowsAppContainer::AccessMode::Read));
    }

    auto assetsPath = QDir::current().absoluteFilePath("assets");
    auto skinsPath = QDir::current().absoluteFilePath("assets/skins");
    FS::ensureFolderPathExists(assetsPath);
    FS::ensureFolderPathExists(skinsPath);
    TRY(m_appContainer->grantFSAccess(assetsPath, WindowsAppContainer::AccessMode::Read));
    TRY(m_appContainer->grantFSAccess(skinsPath, WindowsAppContainer::AccessMode::ReadWrite));

    TRY(m_appContainer->grantFSAccess(m_process.workingDirectory(), WindowsAppContainer::AccessMode::ReadWrite));

    return m_appContainer->finalizeSetup(&m_process);
#else
    return {};
#endif
}

void LauncherPartLaunch::setWorkingDirectory(const QString& wd)
{
    m_process.setWorkingDirectory(wd);
}

void LauncherPartLaunch::proceed()
{
    if (mayProceed) {
        m_parent->instance()->setMinecraftRunning(true);
        QString launchString("launch\n");
        m_process.write(launchString.toUtf8());
        mayProceed = false;
    }
}

bool LauncherPartLaunch::abort()
{
    if (mayProceed) {
        mayProceed = false;
        QString launchString("abort\n");
        m_process.write(launchString.toUtf8());
    } else {
        auto state = m_process.state();
        if (state == LoggedProcess::Running || state == LoggedProcess::Starting) {
            m_process.kill();
        }
    }
    return true;
}
