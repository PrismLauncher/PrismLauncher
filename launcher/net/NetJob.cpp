// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
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

#include "NetJob.h"
#include "Download.h"

auto NetJob::addNetAction(NetAction::Ptr action) -> bool
{
    action->m_index_within_job = m_downloads.size();
    m_downloads.append(action);
    part_info pi;
    m_parts_progress.append(pi);

    partProgress(m_parts_progress.count() - 1, action->getProgress(), action->getTotalProgress());

    if (action->isRunning()) {
        connect(action.get(), &NetAction::succeeded, [this, action]{ partSucceeded(action->index()); });
        connect(action.get(), &NetAction::failed, [this, action](QString){ partFailed(action->index()); });
        connect(action.get(), &NetAction::aborted, [this, action](){ partAborted(action->index()); });
        connect(action.get(), &NetAction::progress, [this, action](qint64 done, qint64 total) { partProgress(action->index(), done, total); });
        connect(action.get(), &NetAction::status, this, &NetJob::status);
    } else {
        m_todo.append(m_parts_progress.size() - 1);
    }

    return true;
}

auto NetJob::canAbort() const -> bool
{
    bool canFullyAbort = true;

    // can abort the downloads on the queue?
    for (auto index : m_todo) {
        auto part = m_downloads[index];
        canFullyAbort &= part->canAbort();
    }
    // can abort the active downloads?
    for (auto index : m_doing) {
        auto part = m_downloads[index];
        canFullyAbort &= part->canAbort();
    }

    return canFullyAbort;
}

void NetJob::executeTask()
{
    // hack that delays early failures so they can be caught easier
    QMetaObject::invokeMethod(this, "startMoreParts", Qt::QueuedConnection);
}

auto NetJob::getFailedFiles() -> QStringList
{
    QStringList failed;
    for (auto index : m_failed) {
        failed.push_back(m_downloads[index]->url().toString());
    }
    failed.sort();
    return failed;
}

auto NetJob::abort() -> bool
{
    bool fullyAborted = true;

    // fail all downloads on the queue
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QSet<int> todoSet(m_todo.begin(), m_todo.end());
    m_failed.unite(todoSet);
#else
    m_failed.unite(m_todo.toSet());
#endif
    m_todo.clear();

    // abort active downloads
    auto toKill = m_doing.values();
    for (auto index : toKill) {
        auto part = m_downloads[index];
        fullyAborted &= part->abort();
    }

    return fullyAborted;
}

void NetJob::partSucceeded(int index)
{
    // do progress. all slots are 1 in size at least
    auto& slot = m_parts_progress[index];
    partProgress(index, slot.total_progress, slot.total_progress);

    m_doing.remove(index);
    m_done.insert(index);
    m_downloads[index].get()->disconnect(this);

    startMoreParts();
}

void NetJob::partFailed(int index)
{
    m_doing.remove(index);

    auto& slot = m_parts_progress[index];
    // Can try 3 times before failing by definitive
    if (slot.failures == 3) {
        m_failed.insert(index);
    } else {
        slot.failures++;
        m_todo.enqueue(index);
    }

    m_downloads[index].get()->disconnect(this);

    startMoreParts();
}

void NetJob::partAborted(int index)
{
    m_aborted = true;

    m_doing.remove(index);
    m_failed.insert(index);
    m_downloads[index].get()->disconnect(this);

    startMoreParts();
}

void NetJob::partProgress(int index, qint64 bytesReceived, qint64 bytesTotal)
{
    auto& slot = m_parts_progress[index];
    slot.current_progress = bytesReceived;
    slot.total_progress = bytesTotal;

    int done = m_done.size();
    int doing = m_doing.size();
    int all = m_parts_progress.size();

    qint64 bytesAll = 0;
    qint64 bytesTotalAll = 0;
    for (auto& partIdx : m_doing) {
        auto part = m_parts_progress[partIdx];
        // do not count parts with unknown/nonsensical total size
        if (part.total_progress <= 0) {
            continue;
        }
        bytesAll += part.current_progress;
        bytesTotalAll += part.total_progress;
    }

    qint64 inprogress = (bytesTotalAll == 0) ? 0 : (bytesAll * 1000) / bytesTotalAll;
    auto current = done * 1000 + doing * inprogress;
    auto current_total = all * 1000;
    // HACK: make sure it never jumps backwards.
    // FAIL: This breaks if the size is not known (or is it something else?) and jumps to 1000, so if it is 1000 reset it to inprogress
    if (m_current_progress == 1000) {
        m_current_progress = inprogress;
    }
    if (m_current_progress > current) {
        current = m_current_progress;
    }
    m_current_progress = current;
    setProgress(current, current_total);
}

void NetJob::startMoreParts()
{
    if (!isRunning()) {
        // this actually makes sense. You can put running m_downloads into a NetJob and then not start it until much later.
        return;
    }

    // OK. We are actively processing tasks, proceed.
    // Check for final conditions if there's nothing in the queue.
    if (!m_todo.size()) {
        if (!m_doing.size()) {
            if (!m_failed.size()) {
                emitSucceeded();
            } else if (m_aborted) {
                emitAborted();
            } else {
                emitFailed(tr("Job '%1' failed to process:\n%2").arg(objectName()).arg(getFailedFiles().join("\n")));
            }
        }
        return;
    }

    // There's work to do, try to start more parts, to a maximum of 6 concurrent ones.
    while (m_doing.size() < 6) {
        if (m_todo.size() == 0)
            return;
        int doThis = m_todo.dequeue();
        m_doing.insert(doThis);

        auto part = m_downloads[doThis];

        // connect signals :D
        connect(part.get(), &NetAction::succeeded, this, [this, part]{ partSucceeded(part->index()); });
        connect(part.get(), &NetAction::failed, this, [this, part](QString){ partFailed(part->index()); });
        connect(part.get(), &NetAction::aborted, this, [this, part]{ partAborted(part->index()); });
        connect(part.get(), &NetAction::progress, this, [this, part](qint64 done, qint64 total) { partProgress(part->index(), done, total); });
        connect(part.get(), &NetAction::status, this, &NetJob::status);

        part->startAction(m_network);
    }
}
