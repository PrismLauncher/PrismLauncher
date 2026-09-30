// SPDX-License-Identifier: GPL-3.0-only
/*
 *  MaterialMC - Minecraft Launcher
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

#pragma once

#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QSet>
#include <QTimer>

#include <optional>

#include "LaunchInteraction.h"

#include "ApiError.h"

class LaunchTask;
class MinecraftInstance;

namespace api {

class ApiRouter;
class TaskTracker;

/**
 * `instances.*`: listing, creation, copy, removal, editing, per-instance settings and launching.
 *
 * Also implements LaunchInteraction: while the web UI is active, Application hands every launch to
 * LaunchController with this object, so the existing pipeline runs unchanged but reports questions
 * and failures as events (`instance.launching|started|stopped|launchFailed`, `minecraft.crashed`)
 * instead of opening Qt dialogs.
 */
class InstanceApi : public QObject, public LaunchInteraction {
    Q_OBJECT
   public:
    InstanceApi(ApiRouter* router, TaskTracker* tasks, QObject* parent = nullptr);
    ~InstanceApi() override;

    static QJsonObject serializeInstance(MinecraftInstance* instance, const QString& state);
    QString stateOf(MinecraftInstance* instance) const;

    // LaunchInteraction
    MinecraftAccountPtr chooseAccount(MinecraftInstance* instance) override;
    bool waitForTask(MinecraftInstance* instance, Task* task, const QString& title) override;
    bool reauthenticate(MinecraftInstance* instance, const MinecraftAccountPtr& account, const QString& reason) override;
    bool confirmDemo(MinecraftInstance* instance, bool hasAccount) override;
    std::optional<QString> offlineName(MinecraftInstance* instance, const QString& suggested, const QString& reason) override;
    bool setupProfile(MinecraftInstance* instance, const MinecraftAccountPtr& account) override;
    void showError(MinecraftInstance* instance, const QString& title, const QString& message) override;
    void showConsole(MinecraftInstance* instance) override;
    bool confirmKill(MinecraftInstance* instance) override;
    void profilerReady(MinecraftInstance* instance, const QString& message) override;
    void launchFinished(MinecraftInstance* instance, bool success, bool aborted, const QString& reason) override;

   signals:
    /** A launch created a new LaunchTask (and with it a new LogModel) for `instanceId`. */
    void launchTaskChanged(const QString& instanceId, LaunchTask* task);

   private:
    void registerMethods();
    void watchInstances();
    void watchInstance(MinecraftInstance* instance);
    void scheduleChanged();
    void recordLaunchError(MinecraftInstance* instance, const ApiError& error);

    QJsonValue list() const;
    QJsonValue get(const QJsonObject& params) const;
    QJsonValue launch(const QJsonObject& params);
    QJsonValue kill(const QJsonObject& params);
    QJsonValue remove(const QJsonObject& params);
    QJsonValue copy(const QJsonObject& params);
    QJsonValue getSettings(const QJsonObject& params) const;
    QJsonValue setSettings(const QJsonObject& params);
    static QJsonObject readSettings(MinecraftInstance* instance);

    ApiRouter* m_router;
    TaskTracker* m_tasks;
    QTimer m_changedTimer;
    QSet<MinecraftInstance*> m_watched;
    /** "launching" | "running" for instances with an active launch controller. */
    QHash<QString, QString> m_states;
    /** Most specific reason a launch was refused, reported when the controller finishes. */
    QHash<QString, ApiError> m_launchErrors;
    /** Instances the user stopped: their "Game crashed." failure is not a crash. */
    QSet<QString> m_killRequested;
};

}  // namespace api
