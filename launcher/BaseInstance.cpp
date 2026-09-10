// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
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

#include "BaseInstance.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

#include "Application.h"
#include "config/GlobalConfig.h"
#include "config/InstanceConfig.h"
#include "launch/LaunchTask.h"

#include "BuildConfig.h"
#include "Commandline.h"

BaseInstance::BaseInstance(std::unique_ptr<InstanceConfigHolder> conf, QString rootDir)
    : m_rootDir(std::move(rootDir)), m_config(std::move(conf))
{
    if (config()->uuid.isEmpty()) {
        regenerateUuid();
    }
}

void BaseInstance::invalidate()
{
    changeStatus(Status::Gone);
    qDebug() << "Instance" << id() << "has been invalidated.";
}

void BaseInstance::changeStatus(BaseInstance::Status newStatus)
{
    Status status = currentStatus();
    if (status != newStatus) {
        m_status = newStatus;
        emit statusChanged(status, newStatus);
    }
}

BaseInstance::Status BaseInstance::currentStatus() const
{
    return m_status;
}

QString BaseInstance::id() const
{
    return QFileInfo(instanceRoot()).fileName();
}

QString BaseInstance::uuid() const
{
    return config()->uuid;
}

void BaseInstance::regenerateUuid()
{
    m_config->update().uuid = QUuid::createUuid().toString(QUuid::Id128);
}

bool BaseInstance::isRunning() const
{
    return m_isRunning;
}

void BaseInstance::setRunning(bool running)
{
    if (running == m_isRunning) {
        return;
    }

    m_isRunning = running;

    emit runningStatusChanged(running);
}

void BaseInstance::setMinecraftRunning(bool running)
{
    const auto gameTime = config()->gameTimeOrGlobal(*APPLICATION->config());
    if (!gameTime.record) {
        return;
    }

    if (running) {
        m_timeStarted = QDateTime::currentDateTime();
        setLastLaunch(m_timeStarted.toMSecsSinceEpoch());
    } else {
        QDateTime timeEnded = QDateTime::currentDateTime();
        qint64 secondsPlayed = m_timeStarted.secsTo(timeEnded);

        auto& conf = m_config->update();
        int64_t current = conf.totalTimePlayed;
        conf.totalTimePlayed = current + secondsPlayed;
        conf.lastTimePlayed = secondsPlayed;

        if (conf.countGameTime) {
            int64_t globalTotal = APPLICATION->config()->totalPlayTime;
            APPLICATION->config().update().totalPlayTime = globalTotal + secondsPlayed;
        }

        emit propertiesChanged();
    }
}

int64_t BaseInstance::totalTimePlayed() const
{
    int64_t current = config()->totalTimePlayed;
    if (m_isRunning) {
        QDateTime timeNow = QDateTime::currentDateTime();
        return current + m_timeStarted.secsTo(timeNow);
    }
    return current;
}

int64_t BaseInstance::lastTimePlayed() const
{
    if (m_isRunning) {
        QDateTime timeNow = QDateTime::currentDateTime();
        return m_timeStarted.secsTo(timeNow);
    }
    return config()->lastTimePlayed;
}

void BaseInstance::resetTimePlayed()
{
    m_config->update().totalTimePlayed = 0;
    m_config->update().lastTimePlayed = 0;
}

QString BaseInstance::instanceRoot() const
{
    return m_rootDir;
}

bool BaseInstance::canLaunch() const
{
    return (!hasVersionBroken() && !isRunning());
}

void BaseInstance::setLastLaunch(qint64 val)
{
    m_config->update().lastLaunchTime = val;
    emit propertiesChanged();
}

void BaseInstance::setIconKey(const QString& val)
{
    m_config->update().iconKey = val;
    emit propertiesChanged();
}

void BaseInstance::setName(const QString& val)
{
    m_config->update().name = val;
    emit propertiesChanged();
}

bool BaseInstance::syncInstanceDirName(const QString& newRoot) const
{
    auto oldRoot = instanceRoot();
    return oldRoot == newRoot || QFile::rename(oldRoot, newRoot);
}

QString BaseInstance::name() const
{
    return config()->name;
}

QString BaseInstance::windowTitle() const
{
    return BuildConfig.LAUNCHER_DISPLAYNAME + ": " + name();
}

// FIXME: why is this here? move it to MinecraftInstance!!!
QStringList BaseInstance::extraArguments()
{
    return Commandline::splitArgs(config()->jvmArgs.value_or(APPLICATION->config()->jvmArgs));
}

LaunchTask* BaseInstance::getLaunchTask()
{
    return m_launchProcess.get();
}

void BaseInstance::updateRuntimeContext()
{
    // NOOP
}

bool BaseInstance::isLegacy() const
{
    return traits().contains("legacyLaunch") || traits().contains("alphaLaunch");
}
