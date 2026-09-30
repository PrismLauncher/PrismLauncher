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

#include "TaskTracker.h"

#include <QDateTime>

#include "ApiError.h"
#include "ApiRouter.h"

namespace api {

namespace {
constexpr int FlushIntervalMs = 150;
constexpr size_t MaxFinishedEntries = 100;
}  // namespace

TaskTracker::TaskTracker(ApiRouter* router, QObject* parent) : QObject(parent), m_router(router)
{
    m_flushTimer.setSingleShot(true);
    m_flushTimer.setInterval(FlushIntervalMs);
    connect(&m_flushTimer, &QTimer::timeout, this, &TaskTracker::flush);
}

QString TaskTracker::start(const Task::Ptr& task, const QString& kind, const QString& title, const QString& instanceId)
{
    const auto id = add(task.get(), task, kind, title, instanceId);
    QMetaObject::invokeMethod(
        this,
        [this, id] {
            auto* entry = find(id);
            if (entry && entry->task && !entry->task->isRunning() && !entry->task->isFinished()) {
                entry->task->start();
            }
        },
        Qt::QueuedConnection);
    return id;
}

QString TaskTracker::observe(const Task::Ptr& task, const QString& kind, const QString& title, const QString& instanceId)
{
    return add(task.get(), task, kind, title, instanceId);
}

QString TaskTracker::observeUnowned(Task* task, const QString& kind, const QString& title, const QString& instanceId)
{
    return add(task, Task::Ptr(), kind, title, instanceId);
}

QString TaskTracker::add(Task* task, Task::Ptr owned, const QString& kind, const QString& title, const QString& instanceId)
{
    Q_ASSERT(task);
    Entry entry;
    entry.id = QStringLiteral("t%1").arg(m_nextId++);
    entry.owned = std::move(owned);
    entry.task = task;
    entry.kind = kind;
    entry.title = title;
    entry.instanceId = instanceId;
    entry.status = task->getStatus();
    entry.details = task->getDetails();
    entry.current = task->getProgress();
    entry.total = task->getTotalProgress();
    entry.canAbort = task->canAbort();
    entry.startedAt = QDateTime::currentMSecsSinceEpoch();
    const auto id = entry.id;
    auto* raw = task;

    connect(raw, &Task::status, this, [this, id](const QString& status) {
        if (auto* e = find(id)) {
            e->status = status;
            markDirty(id);
        }
    });
    connect(raw, &Task::details, this, [this, id](const QString& details) {
        if (auto* e = find(id)) {
            e->details = details;
            markDirty(id);
        }
    });
    connect(raw, &Task::progress, this, [this, id](qint64 current, qint64 total) {
        if (auto* e = find(id)) {
            e->current = current;
            e->total = total;
            markDirty(id);
        }
    });
    connect(raw, &Task::abortStatusChanged, this, [this, id](bool canAbort) {
        if (auto* e = find(id)) {
            e->canAbort = canAbort;
            markDirty(id);
        }
    });
    connect(raw, &Task::succeeded, this, [this, id] { finish(id, "succeeded", {}); });
    connect(raw, &Task::failed, this, [this, id](const QString& reason) { finish(id, "failed", reason); });
    connect(raw, &Task::aborted, this, [this, id] { finish(id, "aborted", tr("Cancelled")); });
    connect(raw, &QObject::destroyed, this, [this, id] { finish(id, "aborted", tr("The task was discarded")); });

    m_entries.push_front(std::move(entry));
    auto& front = m_entries.front();
    front.lastEmit.start();
    emitEntry("download.started", front);

    // Drop the oldest finished entries.
    size_t finished = 0;
    for (auto it = m_entries.begin(); it != m_entries.end();) {
        if (it->state != QLatin1String("running") && ++finished > MaxFinishedEntries) {
            it = m_entries.erase(it);
        } else {
            ++it;
        }
    }
    return id;
}

TaskTracker::Entry* TaskTracker::find(const QString& id)
{
    for (auto& e : m_entries) {
        if (e.id == id) {
            return &e;
        }
    }
    return nullptr;
}

const TaskTracker::Entry* TaskTracker::find(const QString& id) const
{
    for (const auto& e : m_entries) {
        if (e.id == id) {
            return &e;
        }
    }
    return nullptr;
}

bool TaskTracker::contains(const QString& id) const
{
    return find(id) != nullptr;
}

void TaskTracker::markDirty(const QString& id)
{
    m_dirty.insert(id);
    if (!m_flushTimer.isActive()) {
        m_flushTimer.start();
    }
}

void TaskTracker::flush()
{
    const auto dirty = m_dirty;
    m_dirty.clear();
    for (const auto& id : dirty) {
        auto* e = find(id);
        if (e && e->state == QLatin1String("running")) {
            emitEntry("download.progress", *e);
        }
    }
}

void TaskTracker::finish(const QString& id, const QString& state, const QString& reason)
{
    auto* e = find(id);
    if (!e || e->state != QLatin1String("running")) {
        return;
    }
    m_dirty.remove(id);
    e->state = state;
    e->finishedAt = QDateTime::currentMSecsSinceEpoch();
    e->canAbort = false;
    if (state == QLatin1String("failed")) {
        e->error = ApiError("TASK_FAILED", reason.isEmpty() ? tr("The task failed") : reason).toJson();
    } else if (state == QLatin1String("aborted")) {
        e->error = ApiError::cancelled(reason).toJson();
    }
    if (e->task) {
        disconnect(e->task.data(), nullptr, this, nullptr);
    }
    e->task.clear();
    if (e->owned) {
        // Release the task after its signal handlers have returned.
        QMetaObject::invokeMethod(this, [this, id] {
            if (auto* entry = find(id)) {
                entry->owned.reset();
            }
        }, Qt::QueuedConnection);
    }
    emitEntry(state == QLatin1String("succeeded") ? "download.finished" : "download.failed", *e);
}

bool TaskTracker::cancel(const QString& id)
{
    auto* e = find(id);
    if (!e) {
        throw ApiError("TASK_NOT_FOUND", tr("Unknown task '%1'").arg(id));
    }
    if (!e->task || e->state != QLatin1String("running")) {
        return false;
    }
    return e->task->abort();
}

QJsonArray TaskTracker::list() const
{
    QJsonArray out;
    for (const auto& e : m_entries) {
        out.append(serialize(e));
    }
    return out;
}

void TaskTracker::clearFinished()
{
    std::erase_if(m_entries, [](const Entry& e) { return e.state != QLatin1String("running"); });
}

void TaskTracker::emitEntry(const QString& event, Entry& entry)
{
    entry.lastEmit.restart();
    m_router->emitEvent(event, serialize(entry));
}

QJsonObject TaskTracker::serialize(const Entry& e)
{
    return {
        { "id", e.id },
        { "kind", e.kind },
        { "title", e.title },
        { "status", e.status },
        { "details", e.details },
        { "state", e.state },
        { "current", e.current },
        { "total", e.total },
        { "canAbort", e.canAbort },
        { "startedAt", e.startedAt },
        { "finishedAt", e.finishedAt > 0 ? QJsonValue(e.finishedAt) : QJsonValue() },
        { "error", e.error.isEmpty() ? QJsonValue() : QJsonValue(e.error) },
        { "instanceId", e.instanceId.isEmpty() ? QJsonValue() : QJsonValue(e.instanceId) },
    };
}

}  // namespace api
