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

#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QTimer>

#include <deque>

#include "tasks/Task.h"

namespace api {

class ApiRouter;

/**
 * Replaces the Qt ProgressDialog for the web UI: owns backend Tasks started on behalf of the UI and
 * reports their lifecycle as `download.started|progress|finished|failed` events (TaskInfo payloads).
 * Progress events are coalesced to at most one per task every ~150 ms.
 */
class TaskTracker : public QObject {
    Q_OBJECT
   public:
    explicit TaskTracker(ApiRouter* router, QObject* parent = nullptr);

    /**
     * Tracks `task` and starts it on the next event loop iteration (so the RPC reply carrying
     * the id always reaches the UI before any event about the task). Returns the task id.
     */
    QString start(const Task::Ptr& task, const QString& kind, const QString& title, const QString& instanceId = {});

    /** Tracks an already running task that the tracker does not start (e.g. an account refresh) and keeps it alive. */
    QString observe(const Task::Ptr& task, const QString& kind, const QString& title, const QString& instanceId = {});

    /** Tracks a task owned by someone else (e.g. a launch pipeline step); it may be destroyed at any time. */
    QString observeUnowned(Task* task, const QString& kind, const QString& title, const QString& instanceId = {});

    bool cancel(const QString& id);
    bool contains(const QString& id) const;
    QJsonArray list() const;
    void clearFinished();

   private:
    struct Entry {
        QString id;
        Task::Ptr owned;       // set when the tracker keeps the task alive
        QPointer<Task> task;
        QString kind;
        QString title;
        QString instanceId;
        QString state = "running";
        QString status;
        QString details;
        qint64 current = 0;
        qint64 total = 0;
        bool canAbort = false;
        qint64 startedAt = 0;
        qint64 finishedAt = 0;
        QJsonObject error;
        QElapsedTimer lastEmit;
    };

    QString add(Task* task, Task::Ptr owned, const QString& kind, const QString& title, const QString& instanceId);
    Entry* find(const QString& id);
    const Entry* find(const QString& id) const;
    void markDirty(const QString& id);
    void flush();
    void finish(const QString& id, const QString& state, const QString& reason);
    void emitEntry(const QString& event, Entry& entry);
    static QJsonObject serialize(const Entry& entry);

    ApiRouter* m_router;
    std::deque<Entry> m_entries;  // newest first
    QSet<QString> m_dirty;
    QTimer m_flushTimer;
    qint64 m_nextId = 1;
};

}  // namespace api
