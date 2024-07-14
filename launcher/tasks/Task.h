// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PrismLauncher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
 *  Copyright (c) 2023 Rachel Powers <508861+Ryex@users.noreply.github.com>
 *  Copyright (c) 2024 Trial97 <alexandru.tripon97@gmail.com>
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

#include <qobject.h>
#include <QDebug>
#include <QLoggingCategory>
#include <QRunnable>
#include <QUuid>

#include "QObjectPtr.h"

Q_DECLARE_LOGGING_CATEGORY(taskLogC)
class TaskV2 : public QObject, public QRunnable {
    Q_OBJECT
    // public types
   public:
    using Ptr = shared_qobject_ptr<TaskV2>;

    enum State {
        Inactive = 0,
        Running = 1 << 0,
        Paused = 1 << 1,
        Succeeded = 1 << 2,
        Failed = 1 << 3,
        AbortedByUser = 1 << 4,
        Finished = Succeeded | Failed | AbortedByUser,
    };

    enum Capability {
        None = 0,
        Killable = 1,
        Suspendable = 2,
    };
    Q_DECLARE_FLAGS(Capabilities, Capability)
    Q_FLAG(Capabilities)

    // con/des
   public:
    TaskV2(QObject* parent, QtMsgType enableForLevel) : TaskV2(parent, "launcher.task", enableForLevel) {}
    explicit TaskV2(QObject* parent = nullptr, const char* categoryName = "launcher.task", QtMsgType enableForLevel = QtDebugMsg)
        : QObject(parent), m_uuid(QUuid::createUuid()), m_log_cat(categoryName, enableForLevel)
    {
        setAutoDelete(false);
    }
    virtual ~TaskV2()
    {
        if (!isFinished()) {
            emit finished(this);
        }
    };

    // properties
   public:
    QUuid uuid() const { return m_uuid; }
    State state() const { return m_state; }
    QString failReason() const { return m_fail_reason; }
    QStringList warnings() const { return m_warnings; }
    QString status() const { return m_status; }
    QString details() const { return m_details; }
    QString title() const { return m_title; }
    double progress() const { return m_progress; }
    double progressTotal() const { return m_progressTotal; }
    double weight() const { return m_weight; }

    // capabilities are only used to tell UI what actions are supported(not used by the actual Task)
    Capabilities capabilities() const { return m_capabilities; }
    void setCapabilities(Capabilities capabilities) { m_capabilities = capabilities; }

    // functions to check the state
    bool wasSuccessful() const { return m_state & State::Succeeded; }
    bool isRunning() const { return m_state & State::Running; }
    bool isFinished() const { return m_state & State::Finished; }
    bool isPaused() const { return m_state & State::Paused; }
    // is it even used
    virtual bool isMultiStep() const { return false; }

    // this reports all the subtasks associated with the current task
    virtual QList<Ptr> subTasks() const { return {}; }

    // methods
   public:
    // debug operator so we can just log the task
    friend QDebug operator<<(QDebug debug, const TaskV2* v)
    {
        QDebugStateSaver saver(debug);

        debug.nospace() << v->metaObject()->className() << QChar('(');
        auto name = v->objectName();
        if (name.isEmpty()) {
            debug.nospace() << QString("0x%1").arg(reinterpret_cast<quintptr>(v), 0, 16);
        } else {
            debug.nospace() << name;
        }
        debug.nospace() << " ID: " << v->uuid().toString(QUuid::WithoutBraces);
        debug.nospace() << QChar(')');

        return debug;
    }
    // just for association purposes this returns the parent task for the current subtask(null in case it is not a task)
    TaskV2* parentTask() { return dynamic_cast<TaskV2*>(parent()); }

   signals:
    // allways connect if posible
    void finished(TaskV2* job);  // this should be the only exit point of the task and allways be emited at least once per started task
    // progress related signals( the delta is with how much was the value incremented comparaed to previous value)
    void totalChanged(TaskV2* job, double total, double delta);
    void processedChanged(TaskV2* job, double current, double delta);
    // connect only if the task is suspendable
    void resumed(TaskV2* job);
    void paused(TaskV2* job);
    // this should not be connected if the task weight doesn't change (right now not used)
    void weightChanged(TaskV2* job);
    // mostly needed for UI
    void stateChanged(TaskV2* job);
    void started(TaskV2* job);
    void subTaskAdded(TaskV2* job, TaskV2* subTask);
    // maybe deprecate this(warning functionality is not really used)
    void warning(TaskV2* job, const QString& message);

   public slots:
    // QRunnable's interface
    void run() override { start(); }
    // entry point to the task
    bool start()
    {
        switch (m_state) {
            case State::Inactive: {
                qCDebug(m_log_cat) << this << "starting for the first time";
                break;
            }
            case State::AbortedByUser: {
                qCDebug(m_log_cat) << this << "restarting for after being aborted by user";
                break;
            }
            case State::Failed: {
                qCDebug(m_log_cat) << this << "restarting for after failing at first";
                break;
            }
            case State::Succeeded: {
                qCDebug(m_log_cat) << this << "restarting for after succeeding at first";
                break;
            }
            case State::Running: {
                qCWarning(m_log_cat) << "The launcher tried to start " << this << "while it was already running!";
                return false;
            }
            case Paused: {
                qCWarning(m_log_cat) << "The launcher tried to start " << this << "while it was paused!";
                resume();
                return false;
            }
            case Finished:
                break;
        }
        // NOTE: only fall through to here in end states
        setState(State::Running);
        m_warnings.clear();
        m_fail_reason.clear();
        emit started(this);
        // ensure that the executeTask is (partialy)async
        // recomandation executeTask should never block
        QMetaObject::invokeMethod(this, &TaskV2::executeTask, Qt::QueuedConnection);
        return true;
    }

    // wrapper for doPause
    bool pause()
    {  // Don't pause twice.
        if (!isRunning()) {
            qCCritical(m_log_cat) << this << "paused while not running!!!!";
            return false;
        }
        if (doPause()) {
            setState(State::Paused);
            emit paused(this);
            return true;
        }
        return false;
    }
    // wrapper for doResume
    bool resume()
    {  // Don't resume twice.
        if (!isPaused()) {
            qCCritical(m_log_cat) << this << "resumed while not paused!!!!";
            return false;
        }
        if (doResume()) {
            setState(State::Running);
            emit resumed(this);
            return true;
        }
        return false;
    }
    // wrapper for doAbort
    bool abort()
    {  // Don't abort twice.
        if (!isRunning() && !isPaused()) {
            qCCritical(m_log_cat) << this << "aborted while not running!!!!";
            return false;
        }
        if (doAbort()) {
            setState(State::AbortedByUser);
            m_fail_reason = "Aborted.";
            qCDebug(m_log_cat) << this << "aborted.";
            emit finished(this);
            return true;
        }
        return false;
    }

    // used to propagate proggress to the main task
    void propateTotalChanged(TaskV2* job, [[maybe_unused]] double, double delta)
    {
        setProgressTotal(m_progressTotal + delta * job->weight());
    }
    void propateProcessedChanged(TaskV2* job, [[maybe_unused]] double, double delta) { setProgress(m_progress + delta * job->weight()); }
    // only used in case the subtask replaces the main task(it is not needed to be connected)
    void propateState(TaskV2* job)
    {
        m_status = job->status();
        m_details = job->details();
        m_title = job->title();
        emit stateChanged(this);
    }

   protected:
    Q_INVOKABLE virtual void executeTask() = 0;

   protected slots:
    // helper function to finalize the task( emit finished should always only be called from this two functions and abort)
    virtual void emitFailed(QString reason)
    {
        // Don't fail twice.
        if (!isRunning()) {
            qCCritical(m_log_cat) << this << "failed while not running!!!!: " << reason;
            return;
        }
        setState(State::Failed);
        m_fail_reason = reason;
        qCCritical(m_log_cat) << this << "failed: " << reason;
        emit finished(this);
    }

    virtual void emitSucceeded()
    {
        // Don't succeed twice.
        if (!isRunning()) {
            qCCritical(m_log_cat) << this << "succeeded while not running!!!!";
            return;
        }
        setState(State::Succeeded);
        qCDebug(m_log_cat) << this << "succeeded";
        emit finished(this);
    }

    // implement this to suport abort
    virtual bool doAbort() { return false; }
    // implement this to suport pause
    virtual bool doPause() { return false; }
    virtual bool doResume() { return false; }

    // used to report non-critical errors(did not see it used)
    void addWarnings(const QString& msg)
    {
        qWarning(m_log_cat) << msg;
        m_warnings.push_back(msg);
        emit warning(this, msg);
    }
    // used by ConcurrentTask to collect previous warnings
    void addWarnings(QStringList warnings) { m_warnings.append(warnings); }

   protected:
    // helper functions to set the properties(emmits apporopriate signals if changed)
#define SET_FIELD(field, newValue, signal) \
    if (field != newValue) {               \
        field = newValue;                  \
        emit signal;                       \
    }
    void setStatus(const QString& status) { SET_FIELD(m_status, status, stateChanged(this)); }
    void setDetails(const QString& details) { SET_FIELD(m_details, details, stateChanged(this)); }
    void setTitle(const QString& title) { SET_FIELD(m_title, title, stateChanged(this)); }
    void setState(State state) { SET_FIELD(m_state, state, stateChanged(this)); }
    void setProgress(double progress)
    {
        auto delta = progress - m_progress;
        SET_FIELD(m_progress, progress, processedChanged(this, m_progress, delta));
    }

   public:
    void setProgressTotal(double progressTotal)
    {
        auto delta = progressTotal - m_progressTotal;
        SET_FIELD(m_progressTotal, progressTotal, totalChanged(this, m_progressTotal, delta));
    }
    void setWeight(double weight) { SET_FIELD(m_weight, weight, weightChanged(this)); }

   private:
    const QUuid m_uuid;
    const QLoggingCategory m_log_cat;

    Capabilities m_capabilities;

    State m_state = State::Inactive;
    QString m_fail_reason = {};
    QStringList m_warnings = {};
    QString m_status = {};
    QString m_details = {};
    QString m_title = {};
    double m_progress = 0;
    double m_progressTotal = 100;
    double m_weight = 1;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(TaskV2::Capabilities)