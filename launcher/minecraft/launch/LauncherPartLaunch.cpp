// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
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

#include <QStandardPaths>

#include "launch/LaunchTask.h"
#include "minecraft/MinecraftInstance.h"
#include "FileSystem.h"
#include "Commandline.h"
#include "Application.h"

#ifdef Q_OS_LINUX
#include "gamemode_client.h"
#endif

LauncherPartLaunch::LauncherPartLaunch(LaunchTask *parent) : LaunchStep(parent)
{
    auto instance = parent->instance();
    if (instance->settings()->get("CloseAfterLaunch").toBool())
    {
        std::shared_ptr<QMetaObject::Connection> connection{new QMetaObject::Connection};
        *connection = connect(&m_process, &LoggedProcess::log, this, [=](QStringList lines, MessageLevel::Enum level) {
            qDebug() << lines;
            if (lines.filter(QRegularExpression(".*Setting user.+", QRegularExpression::CaseInsensitiveOption)).length() != 0)
            {
                APPLICATION->closeAllWindows();
                disconnect(*connection);
            }
        });
    }

    connect(&m_process, &LoggedProcess::log, this, &LauncherPartLaunch::logLines);
    connect(&m_process, &LoggedProcess::stateChanged, this, &LauncherPartLaunch::on_state);
}

#ifdef Q_OS_WIN
// returns 8.3 file format from long path
#include <windows.h>
QString shortPathName(const QString & file)
{
    auto input = file.toStdWString();
    std::wstring output;
    long length = GetShortPathNameW(input.c_str(), NULL, 0);
    // NOTE: this resizing might seem weird...
    // when GetShortPathNameW fails, it returns length including null character
    // when it succeeds, it returns length excluding null character
    // See: https://msdn.microsoft.com/en-us/library/windows/desktop/aa364989(v=vs.85).aspx
    output.resize(length);
    GetShortPathNameW(input.c_str(),(LPWSTR)output.c_str(),length);
    output.resize(length-1);
    QString ret = QString::fromStdWString(output);
    return ret;
}
#endif

// if the string survives roundtrip through local 8bit encoding...
bool fitsInLocal8bit(const QString & string)
{
    return string == QString::fromLocal8Bit(string.toLocal8Bit());
}

void LauncherPartLaunch::executeTask()
{
    QString newLaunchJar = APPLICATION->getJarPath("NewLaunch.jar");
    if (newLaunchJar.isEmpty())
    {
        const char *reason = QT_TR_NOOP("Launcher library could not be found. Please check your installation.");
        emit logLine(tr(reason), MessageLevel::Fatal);
        emitFailed(tr(reason));
        return;
    }

    auto instance = m_parent->instance();
    std::shared_ptr<MinecraftInstance> minecraftInstance = std::dynamic_pointer_cast<MinecraftInstance>(instance);

    m_launchScript = minecraftInstance->createLaunchScript(m_session, m_serverToJoin);
    QStringList args = minecraftInstance->javaArguments();
    QString allArgs = args.join(", ");
    emit logLine("Java Arguments:\n[" + m_parent->censorPrivateInfo(allArgs) + "]\n\n", MessageLevel::Launcher);

    auto javaPath = FS::ResolveExecutable(instance->settings()->get("JavaPath").toString());

    m_process.setProcessEnvironment(instance->createLaunchEnvironment());

    // make detachable - this will keep the process running even if the object is destroyed
    m_process.setDetachable(true);

    auto natPath = minecraftInstance->getNativePath();
#ifdef Q_OS_WIN
    if (!fitsInLocal8bit(natPath))
    {
        args << "-Djava.library.path=" + shortPathName(natPath);
    }
    else
    {
        args << "-Djava.library.path=" + natPath;
    }
#else
    args << "-Djava.library.path=" + natPath;
#endif

    args << "-cp";
    args << newLaunchJar;
    args << "org.polymc.EntryPoint";

    qDebug() << args.join(' ');

    QString wrapperCommandStr = instance->getWrapperCommand().trimmed();
    if(!wrapperCommandStr.isEmpty())
    {
        auto wrapperArgs = Commandline::splitArgs(wrapperCommandStr);
        auto wrapperCommand = wrapperArgs.takeFirst();
        auto realWrapperCommand = QStandardPaths::findExecutable(wrapperCommand);
        if (realWrapperCommand.isEmpty())
        {
            const char *reason = QT_TR_NOOP("The wrapper command \"%1\" couldn't be found.");
            emit logLine(QString(reason).arg(wrapperCommand), MessageLevel::Fatal);
            emitFailed(tr(reason).arg(wrapperCommand));
            return;
        }
        emit logLine("Wrapper command is:\n" + wrapperCommandStr + "\n\n", MessageLevel::Launcher);
        args.prepend(javaPath);
        m_process.start(wrapperCommand, wrapperArgs + args);
    }
    else
    {
        m_process.start(javaPath, args);
    }

#ifdef Q_OS_LINUX
    if (instance->settings()->get("EnableFeralGamemode").toBool())
    {
        auto pid = m_process.processId();
        if (pid)
        {
            gamemode_request_start_for(pid);
        }
    }
#endif
}

void LauncherPartLaunch::on_state(LoggedProcess::State state)
{
    switch(state)
    {
        case LoggedProcess::FailedToStart:
        {
            //: Error message displayed if instace can't start
            const char *reason = QT_TR_NOOP("Could not launch Minecraft!");
            emit logLine(reason, MessageLevel::Fatal);
            emitFailed(tr(reason));
            return;
        }
        case LoggedProcess::Aborted:
        case LoggedProcess::Crashed:
        {
            m_parent->setPid(-1);
            emitFailed(tr("Game crashed."));
            return;
        }
        case LoggedProcess::Finished:
        {
            auto instance = m_parent->instance();
            if (instance->settings()->get("CloseAfterLaunch").toBool())
                APPLICATION->showMainWindow();

            m_parent->setPid(-1);
            // if the exit code wasn't 0, report this as a crash
            auto exitCode = m_process.exitCode();
            if(exitCode != 0)
            {
                emitFailed(tr("Game crashed."));
                return;
            }
            //FIXME: make this work again
            // m_postlaunchprocess.processEnvironment().insert("INST_EXITCODE", QString(exitCode));
            // run post-exit
            emitSucceeded();
            break;
        }
        case LoggedProcess::Running:
            emit logLine(QString("Minecraft process ID: %1\n\n").arg(m_process.processId()), MessageLevel::Launcher);
            m_parent->setPid(m_process.processId());
            m_parent->instance()->setLastLaunch();
            // send the launch script to the launcher part
            m_process.write(m_launchScript.toUtf8());

            mayProceed = true;
            emit readyForLaunch();
            break;
        default:
            break;
    }
}

void LauncherPartLaunch::setWorkingDirectory(const QString &wd)
{
    m_process.setWorkingDirectory(wd);
}

void LauncherPartLaunch::proceed()
{
    if(mayProceed)
    {
        QString launchString("launch\n");
        m_process.write(launchString.toUtf8());
        mayProceed = false;
    }
}

bool LauncherPartLaunch::abort()
{
    if(mayProceed)
    {
        mayProceed = false;
        QString launchString("abort\n");
        m_process.write(launchString.toUtf8());
    }
    else
    {
        auto state = m_process.state();
        if (state == LoggedProcess::Running || state == LoggedProcess::Starting)
        {
            m_process.kill();
        }
    }
    return true;
}
