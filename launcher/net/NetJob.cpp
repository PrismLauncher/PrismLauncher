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

auto NetJob::addNetAction(NetAction::Ptr action) -> bool
{
    action->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_index_within_job = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_queue.size();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_queue.append(action);

    action->setNetwork(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_network);

    return true;
}

void NetJob::startNext()
{
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_queue.isEmpty() && hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_doing.isEmpty()) {
        // We're finished, check for failures and retry if we can (up to 3 times)
        if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_failed.isEmpty() && hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_try < 3) {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_try += 1;
            while (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_failed.isEmpty())
                hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_queue.enqueue(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_failed.take(*hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_failed.keyBegin()));
        }
    }

    ConcurrentTask::startNext();
}

auto NetJob::size() const -> int
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_queue.size() + hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_doing.size() + hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_done.size();
}

auto NetJob::canAbort() const -> bool
{
    bool canFullyAbort = true;

    // can abort the downloads on the queue?
    for (auto part : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_queue)
        canFullyAbort &= part->canAbort();

    // can abort the active downloads?
    for (auto part : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_doing)
        canFullyAbort &= part->canAbort();

    return canFullyAbort;
}

auto NetJob::abort() -> bool
{
    bool fullyAborted = true;

    // fail all downloads on the queue
    for (auto task : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_queue)
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_failed.insert(task.get(), task);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_queue.clear();

    // abort active downloads
    auto toKill = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_doing.values();
    for (auto part : toKill) {
        fullyAborted &= part->abort();
    }

    if (fullyAborted)
        emitAborted();
    else
        emitFailed(tr("Failed to abort all tasks in the NetJob!"));

    return fullyAborted;
}

auto NetJob::getFailedActions() -> QList<NetAction*>
{
    QList<NetAction*> failed;
    for (auto index : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_failed) {
        failed.push_back(dynamic_cast<NetAction*>(index.get()));
    }
    return failed;
}

auto NetJob::getFailedFiles() -> QList<QString>
{
    QList<QString> failed;
    for (auto index : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_failed) {
        failed.append(static_cast<NetAction*>(index.get())->url().toString());
    }
    return failed;
}

void NetJob::updateState()
{
    emit progress(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_done.count(), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_total_size);
    setStatus(tr("Executing %1 task(s) (%2 out of %3 are done)")
                  .arg(QString::number(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_doing.count()), QString::number(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_done.count()), QString::number(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_total_size)));
}
