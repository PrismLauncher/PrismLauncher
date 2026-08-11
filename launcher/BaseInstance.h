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

#pragma once
#include <cassert>

#include <QDataStream>
#include <QDateTime>
#include <QList>
#include <QMenu>
#include <QObject>
#include <QProcess>
#include <QSet>
#include <cstdint>
#include "QObjectPtr.h"

#include "minecraft/auth/MinecraftAccount.h"

#include "RuntimeContext.h"
#include "minecraft/launch/MinecraftTarget.h"

class InstanceConfigHolder;
class QDir;
class Task;
class LaunchTask;
class BaseInstance;

/*!
 * \brief Base class for instances.
 * This class implements many functions that are common between instances and
 * provides a standard interface for all instances.
 *
 * To create a new instance type, create a new class inheriting from this class
 * and implement the pure virtual functions.
 */
class BaseInstance : public QObject {
    Q_OBJECT
   protected:
    /// no-touchy!
    BaseInstance(std::unique_ptr<InstanceConfigHolder> conf, QString rootDir);

   public: /* types */
    enum class Status : std::uint8_t {
        Present,
        Gone  // either nuked or invalidated
    };

   public:
    /// virtual destructor to make sure the destruction is COMPLETE
    ~BaseInstance() override = default;

    virtual void saveNow() = 0;

    /***
     * the instance has been invalidated - it is no longer tracked by the launcher for some reason,
     * but it has not necessarily been deleted.
     *
     * Happens when the instance folder changes to some other location, or the instance is removed by external means.
     */
    void invalidate();

    /// The instance's ID. The ID SHALL be determined by LAUNCHER internally. The ID IS guaranteed to
    /// be unique.
    QString id() const;
    QString uuid() const;
    void regenerateUuid();

    void setMinecraftRunning(bool running);
    void setRunning(bool running);
    bool isRunning() const;
    int64_t totalTimePlayed() const;
    int64_t lastTimePlayed() const;
    void resetTimePlayed();

    /// Path to the instance's root directory.
    QString instanceRoot() const;

    /// Path to the instance's game root directory.
    virtual QString gameRoot() const { return instanceRoot(); }

    /// Path to the instance's mods directory.
    virtual QString modsRoot() const = 0;

    QString name() const;
    void setName(const QString& val);

    /// Sync name and rename instance dir accordingly; returns true if successful
    bool syncInstanceDirName(const QString& newRoot) const;

    /// Value used for instance window titles
    QString windowTitle() const;

    void setIconKey(const QString& val);

    virtual QStringList extraArguments();

    /// Traits. Normally inside the version, depends on instance implementation.
    virtual QSet<QString> traits() const = 0;

    /// Sets the last launched time to 'val' milliseconds since epoch
    void setLastLaunch(qint64 val = QDateTime::currentMSecsSinceEpoch());

    InstanceConfigHolder& config() { return *m_config; }

    const InstanceConfigHolder& config() const { return *m_config; }

    /// returns a valid update task
    virtual QList<Task::Ptr> createUpdateTask() = 0;

    /// returns a valid launcher (task container)
    virtual LaunchTask* createLaunchTask(AuthSessionPtr account, MinecraftTarget::Ptr targetToJoin) = 0;

    /// returns the current launch task (if any)
    LaunchTask* getLaunchTask();

    /*!
     * Create envrironment variables for running the instance
     */
    virtual QProcessEnvironment createEnvironment() = 0;
    virtual QProcessEnvironment createLaunchEnvironment() = 0;

    /*!
     * Returns the root folder to use for looking up log files
     */
    virtual QStringList getLogFileSearchPaths() = 0;

    virtual QString getStatusbarDescription() = 0;

    /// FIXME: this really should be elsewhere...
    virtual QString instanceConfigFolder() const = 0;

    /// get variables this instance exports
    virtual QMap<QString, QString> getVariables() = 0;

    virtual void updateRuntimeContext();
    RuntimeContext runtimeContext() const { return m_runtimeContext; }

    bool hasVersionBroken() const { return m_hasBrokenVersion; }
    void setVersionBroken(bool value)
    {
        if (m_hasBrokenVersion != value) {
            m_hasBrokenVersion = value;
            emit propertiesChanged();
        }
    }

    bool hasUpdateAvailable() const { return m_hasUpdate; }
    void setUpdateAvailable(bool value)
    {
        if (m_hasUpdate != value) {
            m_hasUpdate = value;
            emit propertiesChanged();
        }
    }

    bool hasCrashed() const { return m_crashed; }
    void setCrashed(bool value)
    {
        if (m_crashed != value) {
            m_crashed = value;
            emit propertiesChanged();
        }
    }

    virtual bool canLaunch() const;
    virtual bool canEdit() const = 0;
    virtual bool canExport() const = 0;

    virtual void populateLaunchMenu(QMenu* menu) = 0;

    /**
     * 'print' a verbose description of the instance into a QStringList
     */
    virtual QStringList verboseDescription(AuthSessionPtr session, MinecraftTarget::Ptr targetToJoin) = 0;

    Status currentStatus() const;

    bool isLegacy() const;

   protected:
    void changeStatus(Status newStatus);

   signals:
    /*!
     * \brief Signal emitted when properties relevant to the instance view change
     */
    void propertiesChanged();

    void launchTaskChanged(LaunchTask*);

    void runningStatusChanged(bool running);

    void profilerChanged();

    void statusChanged(Status from, Status to);

   protected: /* data */
    QString m_rootDir;
    std::unique_ptr<InstanceConfigHolder> m_config;
    // InstanceFlags m_flags;
    bool m_isRunning = false;
    std::unique_ptr<LaunchTask> m_launchProcess;
    QDateTime m_timeStarted;
    RuntimeContext m_runtimeContext;

   private: /* data */
    QString m_uuid;
    Status m_status = Status::Present;
    bool m_crashed = false;
    bool m_hasUpdate = false;
    bool m_hasBrokenVersion = false;
};

Q_DECLARE_METATYPE(shared_qobject_ptr<BaseInstance>)
// Q_DECLARE_METATYPE(BaseInstance::InstanceFlag)
// Q_DECLARE_OPERATORS_FOR_FLAGS(BaseInstance::InstanceFlags)
