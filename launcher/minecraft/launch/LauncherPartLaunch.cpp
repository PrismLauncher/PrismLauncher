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
#include "FileSystem.h"
#include "Json.h"
#include "launch/LaunchTask.h"
#include "minecraft/MinecraftInstance.h"

#if defined(Q_OS_LINUX) && defined(ENABLE_GAMEMODE)
#include "gamemode_client.h"
#endif

using namespace std::literals;

LauncherPartLaunch::LauncherPartLaunch(LaunchTask* parent)
    : LaunchStep(parent)
    , m_process(parent->instance()->getJavaVersion().defaultsToUtf8() ? QStringConverter::Utf8 : QStringConverter::System)
{
    // if (parent->instance()->settings()->get("CloseAfterLaunch").toBool()) {
    //     static const QRegularExpression s_settingUser(".*Setting user.+", QRegularExpression::CaseInsensitiveOption);
    //     std::shared_ptr<QMetaObject::Connection> connection{ new QMetaObject::Connection };
    //     *connection =
    //         connect(&m_process, &LoggedProcess::log, this, [connection](const QStringList& lines, [[maybe_unused]] MessageLevel level) {
    //             qDebug() << lines;
    //             if (lines.filter(s_settingUser).length() != 0) {
    //                 APPLICATION->closeAllWindows();
    //                 disconnect(*connection);
    //             }
    //         });
    // }
    //
    // connect(&m_process, &LoggedProcess::log, this, &LauncherPartLaunch::logLines);
    // connect(&m_process, &LoggedProcess::stateChanged, this, &LauncherPartLaunch::on_state);
    connect(&m_process, &SandboxedProcess::stdoutRead, this,
            [this](const QStringList& lines) { emit logLines(lines, MessageLevel::StdOut); });
    connect(&m_process, &SandboxedProcess::stderrRead, this,
            [this](const QStringList& lines) { emit logLines(lines, MessageLevel::StdErr); });
    connect(&m_process, &SandboxedProcess::started, this, &LauncherPartLaunch::onStart);
    connect(&m_process, &SandboxedProcess::finished, this, &LauncherPartLaunch::onFinish);
    connect(&m_process, &SandboxedProcess::failedToStart, this, &LauncherPartLaunch::onFailedToStart);
}

void LauncherPartLaunch::executeTask()
{
    QString jarPath = APPLICATION->getJarPath("NewLaunch.jar");
    if (jarPath.isEmpty()) {
        const char* reason = QT_TR_NOOP("Launcher library could not be found. Please check your installation.");
        emit logLine(tr(reason), MessageLevel::Fatal);
        emitFailed(tr(reason));
        return;
    }

    auto* instance = m_parent->instance();

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
    std::vector<std::string> args;

    QStringList javaArgs = instance->javaArguments();
    emit logLine("Java arguments:\n  " + m_parent->censorPrivateInfo(javaArgs.join(" ")) + "\n", MessageLevel::Launcher);

    for (const auto& javaArg : javaArgs) {
        args.push_back(javaArg.toStdString());
    }

    auto javaPath = FS::ResolveExecutable(instance->settings()->get("JavaPath").toString());

    std::vector<std::pair<std::string, std::string>> env;
    const auto launchEnv = instance->createLaunchEnvironment();
    for (const auto& key : launchEnv.keys()) {
        env.emplace_back(key.toStdString(), launchEnv.value(key).toStdString());
    }

    // make detachable - this will keep the process running even if the object is destroyed
    // FIXME: implement in SandboxedProcess
    // m_process.setDetachable(true);

    auto classPath = instance->getClassPath();
    classPath.prepend(jarPath);

    if (!legacyJarPath.isEmpty()) {
        classPath.prepend(legacyJarPath);
    }

    auto natPath = instance->getNativePath();
#ifdef Q_OS_WIN
    natPath = FS::getPathNameInLocal8bit(natPath);
#endif
    args.push_back("-Djava.library.path="s + natPath.toStdString());

    args.emplace_back("-cp");
#ifdef Q_OS_WIN
    std::string joinedClassPath;
    QStringList processed;
    for (const auto& item : classPath) {
        if (!joinedClassPath.empty()) {
            joinedClassPath += ';';
        }
        joinedClassPath += FS::getPathNameInLocal8bit(item).toStdString();
    }
    args.push_back(joinedClassPath);
#else
    args.push_back(classPath.join(':').toStdString());
#endif
    args.emplace_back("org.prismlauncher.EntryPoint");

    QString wrapperCommandStr = instance->getWrapperCommand().trimmed();
    if (!wrapperCommandStr.isEmpty()) {
        auto wrapperArgs = m_parent->substituteVariables(wrapperCommandStr);
        auto wrapperCommand = wrapperArgs.takeFirst();
        auto realWrapperCommand = QStandardPaths::findExecutable(wrapperCommand);
        if (realWrapperCommand.isEmpty()) {
            const char* reason = QT_TR_NOOP("The wrapper command \"%1\" couldn't be found.");
            emit logLine(QString(reason).arg(wrapperCommand), MessageLevel::Fatal);
            emitFailed(tr(reason).arg(wrapperCommand));
            return;
        }
        emit logLine("Wrapper command is:\n" + wrapperCommandStr + "\n\n", MessageLevel::Launcher);
        args.insert(args.begin(), javaPath.toStdString());
        auto iter = args.begin();
        for (const auto& arg : wrapperArgs) {
            iter = args.insert(iter, arg.toStdString());
        }
    }

    auto command = !wrapperCommandStr.isEmpty() ? wrapperCommandStr.toStdString() : javaPath.toStdString();

    std::optional<SandboxedProcess::SandboxOptions> sandbox = std::nullopt;

    if (instance->settings()->get("SandboxEnabled").toBool()) {
        std::vector<std::string> readOnlyPaths;
        std::vector<std::string> readWritePaths;

        const auto access = Json::toMap(instance->settings()->get("SandboxFileSystemAccess").toString());
        for (const auto& [path, value] : access.asKeyValueRange()) {
            QString accessLevel = value.toString();

            if (accessLevel == "ro") {
                readOnlyPaths.push_back(path.toStdString());
            } else if (accessLevel == "rw") {
                readWritePaths.push_back(path.toStdString());
            } else {
                qWarning() << "What do you call this??!" << value;
            }
        }

        readOnlyPaths.push_back(QFileInfo(jarPath).absolutePath().toStdString());

        sandbox = SandboxedProcess::SandboxOptions{
            .isJvm = true,
            .allowNetwork = instance->settings()->get("SandboxNetworkAccess").toBool(),
            // FIXME: this should probably be per instance
            .appContainerName = "PrismLauncherSandbox",
            .appContainerDescription = "Sandbox for Prism Launcher instances.",
            .readOnlyPaths = std::move(readOnlyPaths),
            .readWritePaths = std::move(readWritePaths),
        };
    }

    m_process.start({
        .command = std::move(command),
        .args = std::move(args),
        .workingDir = instance->gameRoot().toStdString(),
        .env = std::move(env),
        .sandbox = sandbox,
    });
}

void LauncherPartLaunch::onStart(std::uint32_t pid)
{
    m_killing = false;

    emit logLine(QString("Minecraft process ID: %1\n\n").arg(pid), MessageLevel::Launcher);
    m_parent->setPid(pid);
#if defined(Q_OS_LINUX) && defined(ENABLE_GAMEMODE)
    if (pid > 0 && instance->settings()->get("EnableFeralGamemode").toBool() &&
        APPLICATION->capabilities() & Application::SupportsGameMode) {
        gamemode_request_start_for(static_cast<pid_t>(pid));
    }
#endif
    // send the launch script to the launcher part
    (void)m_process.write(m_launchScript.toUtf8());

    mayProceed = true;
    emit readyForLaunch();
}

void LauncherPartLaunch::onFinish(std::optional<std::int32_t> exitCode)
{
    auto* instance = m_parent->instance();
    if (instance->settings()->get("CloseAfterLaunch").toBool()) {
        APPLICATION->showMainWindow();
    }

    m_parent->setPid(-1);
    m_parent->instance()->setMinecraftRunning(false);
    if (m_killing) {
        const auto msg = tr("Process was killed by user");
        emit logLine(msg, MessageLevel::Error);
        emitFailed(msg);
        return;
    }

    if (!exitCode.has_value() || exitCode.value() != 0) {
        const auto msg = exitCode.has_value() ? tr("Game crashed with exit code %1").arg(exitCode.value()) : tr("Game crashed");
        emit logLine(msg, MessageLevel::Error);
        emitFailed(msg);
        return;
    }

    // FIXME: make this work again
    //  m_postlaunchprocess.processEnvironment().insert("INST_EXITCODE", QString(exitCode));
    //  run post-exit
    emit logLine(tr("Game exited with code %1").arg(exitCode.value()), MessageLevel::Launcher);
    emitSucceeded();
}

void LauncherPartLaunch::onFailedToStart(const QString& errorMessage)
{
    //: Error message displayed if instace can't start
    const char* reason = QT_TR_NOOP("Could not launch Minecraft: %1");
    emit logLine(QString(reason).arg(errorMessage), MessageLevel::Fatal);
    emitFailed(tr(reason).arg(errorMessage));
}

void LauncherPartLaunch::proceed()
{
    if (mayProceed) {
        m_parent->instance()->setMinecraftRunning(true);
        QString launchString("launch\n");
        (void)m_process.write(launchString.toUtf8());
        mayProceed = false;
    }
}

bool LauncherPartLaunch::abort()
{
    if (mayProceed) {
        mayProceed = false;
        QString launchString("abort\n");
        (void)m_process.write(launchString.toUtf8());
    } else {
        m_killing = true;
        m_process.kill();
    }
    return true;
}
