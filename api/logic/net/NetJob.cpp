/* Copyright 2013-2019 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "NetJob.h"
#include "Download.h"

#include <QDebug>

void NetJob::partSucceeded(int index)
{
    // do progress. all slots are 1 in size at least
    auto &slot = parts_progress[index];
    partProgress(index, slot.total_progress, slot.total_progress);

    m_doing.remove(index);
    m_done.insert(index);
    downloads[index].get()->disconnect(this);
    startMoreParts();
}

void NetJob::partFailed(int index)
{
    m_doing.remove(index);
    auto &slot = parts_progress[index];
    if (slot.failures == 3)
    {
        m_failed.insert(index);
    }
    else
    {
        slot.failures++;
        m_todo.enqueue(index);
    }
    downloads[index].get()->disconnect(this);
    startMoreParts();
}

void NetJob::partAborted(int index)
{
    m_aborted = true;
    m_doing.remove(index);
    m_failed.insert(index);
    downloads[index].get()->disconnect(this);
    startMoreParts();
}

void NetJob::partProgress(int index, qint64 bytesReceived, qint64 bytesTotal)
{
    auto &slot = parts_progress[index];
    slot.current_progress = bytesReceived;
    slot.total_progress = bytesTotal;

    int done = m_done.size();
    int doing = m_doing.size();
    int all = parts_progress.size();

    qint64 bytesAll = 0;
    qint64 bytesTotalAll = 0;
    for(auto & partIdx: m_doing)
    {
        auto part = parts_progress[partIdx];
        // do not count parts with unknown/nonsensical total size
        if(part.total_progress <= 0)
        {
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
    if(m_current_progress == 1000) {
        m_current_progress = inprogress;
    }
    if(m_current_progress > current)
    {
        current = m_current_progress;
    }
    m_current_progress = current;
    setProgress(current, current_total);
}

void NetJob::executeTask()
{
    // hack that delays early failures so they can be caught easier
    QMetaObject::invokeMethod(this, "startMoreParts", Qt::QueuedConnection);
}

void NetJob::startMoreParts()
{
    if(!isRunning())
    {
        // this actually makes sense. You can put running downloads into a NetJob and then not start it until much later.
        return;
    }
    // OK. We are actively processing tasks, proceed.
    // Check for final conditions if there's nothing in the queue.
    if(!m_todo.size())
    {
        if(!m_doing.size())
        {
            if(!m_failed.size())
            {
                emitSucceeded();
            }
            else if(m_aborted)
            {
                emitAborted();
            }
            else
            {
                emitFailed(tr("Job '%1' failed to process:\n%2").arg(objectName()).arg(getFailedFiles().join("\n")));
            }
        }
        return;
    }
    // There's work to do, try to start more parts.
    while (m_doing.size() < 6)
    {
        if(!m_todo.size())
            return;
        int doThis = m_todo.dequeue();
        m_doing.insert(doThis);
        auto part = downloads[doThis];
        // connect signals :D
        connect(part.get(), SIGNAL(succeeded(int)), SLOT(partSucceeded(int)));
        connect(part.get(), SIGNAL(failed(int)), SLOT(partFailed(int)));
        connect(part.get(), SIGNAL(aborted(int)), SLOT(partAborted(int)));
        connect(part.get(), SIGNAL(netActionProgress(int, qint64, qint64)),
                SLOT(partProgress(int, qint64, qint64)));
        part->start();
    }
}


QStringList NetJob::getFailedFiles()
{
    QStringList failed;
    for (auto index: m_failed)
    {
        failed.push_back(downloads[index]->url().toString());
    }
    failed.sort();
    return failed;
}

bool NetJob::canAbort() const
{
    bool canFullyAbort = true;
    // can abort the waiting?
    for(auto index: m_todo)
    {
        auto part = downloads[index];
        canFullyAbort &= part->canAbort();
    }
    // can abort the active?
    for(auto index: m_doing)
    {
        auto part = downloads[index];
        canFullyAbort &= part->canAbort();
    }
    return canFullyAbort;
}

bool NetJob::abort()
{
    bool fullyAborted = true;
    // fail all waiting
    m_failed.unite(m_todo.toSet());
    m_todo.clear();
    // abort active
    auto toKill = m_doing.toList();
    for(auto index: toKill)
    {
        auto part = downloads[index];
        fullyAborted &= part->abort();
    }
    return fullyAborted;
}

bool NetJob::addNetAction(NetActionPtr action)
{
    action->m_index_within_job = downloads.size();
    downloads.append(action);
    part_info pi;
    parts_progress.append(pi);
    partProgress(parts_progress.count() - 1, action->currentProgress(), action->totalProgress());

    if(action->isRunning())
    {
        connect(action.get(), SIGNAL(succeeded(int)), SLOT(partSucceeded(int)));
        connect(action.get(), SIGNAL(failed(int)), SLOT(partFailed(int)));
        connect(action.get(), SIGNAL(netActionProgress(int, qint64, qint64)), SLOT(partProgress(int, qint64, qint64)));
    }
    else
    {
        m_todo.append(parts_progress.size() - 1);
    }
    return true;
}

NetJob::~NetJob() = default;
