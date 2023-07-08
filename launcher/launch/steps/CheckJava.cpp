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

#include "CheckJava.h"
#include "java/JavaUtils.h"
#include <launch/LaunchTask.h>
#include <FileSystem.h>
#include <QStandardPaths>
#include <QFileInfo>
#include <sys.h>

void CheckJava::executeTask()
{
    auto instance = m_parent->instance();
    auto settings = instance->settings();
    m_javaPath = FS::ResolveExecutable(settings->get("JavaPath").toString());
    bool perInstance = settings->get("OverrideJava").toBool() || settings->get("OverrideJavaLocation").toBool();

    auto realJavaPath = QStandardPaths::findExecutable(m_javaPath);
    if (realJavaPath.isEmpty())
    {
        if (perInstance)
        {
            emit logLine(
                QString("The java binary \"%1\" couldn't be found. Please fix the java path "
                   "override in the instance's settings or disable it.").arg(m_javaPath),
                MessageLevel::Warning);
        }
        else
        {
            emit logLine(QString("The java binary \"%1\" couldn't be found. Please set up java in "
                            "the settings.").arg(m_javaPath),
                         MessageLevel::Warning);
        }
        emitFailed(QString("Java path is not valid."));
        return;
    }
    else
    {
        emit logLine("Java path is:\n" + m_javaPath + "\n\n", MessageLevel::Launcher);
    }

    if (JavaUtils::getJavaCheckPath().isEmpty())
    {
        const char *reason = QT_TR_NOOP("Java checker library could not be found. Please check your installation.");
        emit logLine(tr(reason), MessageLevel::Fatal);
        emitFailed(tr(reason));
        return;
    }

    QFileInfo javaInfo(realJavaPath);
    qint64 javaUnixTime = javaInfo.lastModified().toMSecsSinceEpoch();
    auto storedSignature = settings->get("JavaSignature").toString();
    auto storedArchitecture = settings->get("JavaArchitecture").toString();
    auto storedRealArchitecture = settings->get("JavaRealArchitecture").toString();
    auto storedVersion = settings->get("JavaVersion").toString();
    auto storedVendor = settings->get("JavaVendor").toString();

    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(QByteArray::number(javaUnixTime));
    hash.addData(m_javaPath.toUtf8());
    m_javaSignature = hash.result().toHex();

    // if timestamps are not the same, or something is missing, check!
    if (m_javaSignature != storedSignature || storedVersion.size() == 0
        || storedArchitecture.size() == 0 || storedRealArchitecture.size() == 0
        || storedVendor.size() == 0)
    {
        m_JavaChecker.reset(new JavaChecker);
        emit logLine(QString("Checking Java version..."), MessageLevel::Launcher);
        connect(m_JavaChecker.get(), &JavaChecker::checkFinished, this, &CheckJava::checkJavaFinished);
        m_JavaChecker->m_path = realJavaPath;
        m_JavaChecker->performCheck();
        return;
    }
    else
    {
        auto verString = instance->settings()->get("JavaVersion").toString();
        auto archString = instance->settings()->get("JavaArchitecture").toString();
        auto realArchString = settings->get("JavaRealArchitecture").toString();
        auto vendorString = instance->settings()->get("JavaVendor").toString();
        printJavaInfo(verString, archString, realArchString, vendorString);
    }
    emitSucceeded();
}

void CheckJava::checkJavaFinished(JavaCheckResult result)
{
    switch (result.validity)
    {
        case JavaCheckResult::Validity::Errored:
        {
            // Error message displayed if java can't start
            emit logLine(QString("Could not start java:"), MessageLevel::Error);
            emit logLines(result.errorLog.split('\n'), MessageLevel::Error);
            emit logLine(QString("\nCheck your Java settings."), MessageLevel::Launcher);
            emitFailed(QString("Could not start java!"));
            return;
        }
        case JavaCheckResult::Validity::ReturnedInvalidData:
        {
            emit logLine(QString("Java checker returned some invalid data we don't understand:"), MessageLevel::Error);
            emit logLines(result.outLog.split('\n'), MessageLevel::Warning);
            emit logLine("\nMinecraft might not start properly.", MessageLevel::Launcher);
            emitSucceeded();
            return;
        }
        case JavaCheckResult::Validity::Valid:
        {
            auto instance = m_parent->instance();
            printJavaInfo(result.javaVersion.toString(), result.mojangPlatform, result.realPlatform, result.javaVendor);
            instance->settings()->set("JavaVersion", result.javaVersion.toString());
            instance->settings()->set("JavaArchitecture", result.mojangPlatform);
            instance->settings()->set("JavaRealArchitecture", result.realPlatform);
            instance->settings()->set("JavaVendor", result.javaVendor);
            instance->settings()->set("JavaSignature", m_javaSignature);
            emitSucceeded();
            return;
        }
    }
}

void CheckJava::printJavaInfo(const QString& version, const QString& architecture, const QString& realArchitecture, const QString & vendor)
{
    emit logLine(QString("Java is version %1, using %2 (%3) architecture, from %4.\n\n")
                    .arg(version, architecture, realArchitecture, vendor), MessageLevel::Launcher);
}
