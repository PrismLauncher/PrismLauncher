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

#include "JavaChecker.h"

#include <QFile>
#include <QProcess>
#include <QMap>
#include <QDebug>

#include "JavaUtils.h"
#include "FileSystem.h"
#include "Commandline.h"
#include "Application.h"

JavaChecker::JavaChecker(QObject *parent) : QObject(parent)
{
}

void JavaChecker::performCheck()
{
    QString checkerJar = JavaUtils::getJavaCheckPath();

    if (checkerJar.isEmpty())
    {
        qDebug() << "Java checker library could not be found. Please check your installation.";
        return;
    }

    QStringList args;

    process.reset(new QProcess());
    if(m_args.size())
    {
        auto extraArgs = Commandline::splitArgs(m_args);
        args.append(extraArgs);
    }
    if(m_minMem != 0)
    {
        args << QString("-Xms%1m").arg(m_minMem);
    }
    if(m_maxMem != 0)
    {
        args << QString("-Xmx%1m").arg(m_maxMem);
    }
    if(m_permGen != 64)
    {
        args << QString("-XX:PermSize=%1m").arg(m_permGen);
    }

    args.append({"-jar", checkerJar});
    process->setArguments(args);
    process->setProgram(m_path);
    process->setProcessChannelMode(QProcess::SeparateChannels);
    process->setProcessEnvironment(CleanEnviroment());
    qDebug() << "Running java checker:" << m_path << args.join(" ");

    connect(process.get(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &JavaChecker::finished);
    connect(process.get(), &QProcess::errorOccurred, this, &JavaChecker::error);
    connect(process.get(), &QProcess::readyReadStandardOutput, this, &JavaChecker::stdoutReady);
    connect(process.get(), &QProcess::readyReadStandardError, this, &JavaChecker::stderrReady);
    connect(&killTimer, &QTimer::timeout, this, &JavaChecker::timeout);
    killTimer.setSingleShot(true);
    killTimer.start(15000);
    process->start();
}

void JavaChecker::stdoutReady()
{
    QByteArray data = process->readAllStandardOutput();
    QString added = QString::fromLocal8Bit(data);
    added.remove('\r');
    m_stdout += added;
}

void JavaChecker::stderrReady()
{
    QByteArray data = process->readAllStandardError();
    QString added = QString::fromLocal8Bit(data);
    added.remove('\r');
    m_stderr += added;
}

void JavaChecker::finished(int exitcode, QProcess::ExitStatus status)
{
    killTimer.stop();
    QProcessPtr _process = process;
    process.reset();

    JavaCheckResult result;
    {
        result.path = m_path;
        result.id = m_id;
    }
    result.errorLog = m_stderr;
    result.outLog = m_stdout;
    qDebug() << "STDOUT" << m_stdout;
    qWarning() << "STDERR" << m_stderr;
    qDebug() << "Java checker finished with status" << status << "exit code" << exitcode;

    if (status == QProcess::CrashExit || exitcode == 1)
    {
        result.validity = JavaCheckResult::Validity::Errored;
        emit checkFinished(result);
        return;
    }

    bool success = true;

    QMap<QString, QString> results;

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QStringList lines = m_stdout.split("\n", Qt::SkipEmptyParts);
#else
    QStringList lines = m_stdout.split("\n", QString::SkipEmptyParts);
#endif
    for(QString line : lines)
    {
        line = line.trimmed();
        // NOTE: workaround for GH-4125, where garbage is getting printed into stdout on bedrock linux
        if (line.contains("/bedrock/strata")) {
            continue;
        }

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        auto parts = line.split('=', Qt::SkipEmptyParts);
#else
        auto parts = line.split('=', QString::SkipEmptyParts);
#endif
        if(parts.size() != 2 || parts[0].isEmpty() || parts[1].isEmpty())
        {
            continue;
        }
        else
        {
            results.insert(parts[0], parts[1]);
        }
    }

    if(!results.contains("os.arch") || !results.contains("java.version") || !results.contains("java.vendor") || !success)
    {
        result.validity = JavaCheckResult::Validity::ReturnedInvalidData;
        emit checkFinished(result);
        return;
    }

    auto os_arch = results["os.arch"];
    auto java_version = results["java.version"];
    auto java_vendor = results["java.vendor"];
    bool is_64 = os_arch == "x86_64" || os_arch == "amd64" || os_arch == "aarch64" || os_arch == "arm64";


    result.validity = JavaCheckResult::Validity::Valid;
    result.is_64bit = is_64;
    result.mojangPlatform = is_64 ? "64" : "32";
    result.realPlatform = os_arch;
    result.javaVersion = java_version;
    result.javaVendor = java_vendor;
    qDebug() << "Java checker succeeded.";
    emit checkFinished(result);
}

void JavaChecker::error(QProcess::ProcessError err)
{
    if(err == QProcess::FailedToStart)
    {
        qDebug() << "Java checker has failed to start.";
        qDebug() << "Process environment:";
        qDebug() << process->environment();
        qDebug() << "Native environment:";
        qDebug() << QProcessEnvironment::systemEnvironment().toStringList();
        killTimer.stop();
        JavaCheckResult result;
        {
            result.path = m_path;
            result.id = m_id;
        }

        emit checkFinished(result);
        return;
    }
}

void JavaChecker::timeout()
{
    // NO MERCY. NO ABUSE.
    if(process)
    {
        qDebug() << "Java checker has been killed by timeout.";
        process->kill();
    }
}
