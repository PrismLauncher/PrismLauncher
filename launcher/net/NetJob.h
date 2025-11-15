// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
 *  Copyright (C) 2023 Rachel Powers <508861+Ryex@users.noreply.github.com>
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

#include "net/NetRequest.h"
#include "tasks/ConcurrentTask.h"

// Those are included so that they are also included by anyone using NetJob
#include <queue>

#include "net/Download.h"
#include "net/HttpMetaCache.h"

class NetJob final : public Task {
    Q_OBJECT

   public:
    using Ptr = shared_qobject_ptr<NetJob>;

    explicit NetJob(QString jobName, int maxConcurrent = -1);
    ~NetJob() override;

    bool canAbort() const override { return true; }
    void addNetAction(Net::NetRequest::Ptr action);
    size_t requestsSize() const;
    std::deque<Net::NetRequest::Ptr> getFailedRequests() const;

    void setAskRetry(bool askRetry);

    void setSuppressSucceeded(bool suppressSucceeded);

    bool isMultiStep() const override;

   public slots:
    // Qt can't handle auto at the start for some reason?
    bool abort() override;
    void emitFailed(QString reason) override;

   protected:
    void executeTask() override;

   private:
    bool isOnline() const;
    void perform();
    Net::NetRequest::Ptr findRequestByHandle(const CURL* handle);
    void onAllTransfersComplete();

   private:
    QString m_jobName{};
    int m_maxConcurrent;

    std::unique_ptr<CURLM, decltype(&curl_multi_cleanup)> m_curl{curl_multi_init(), curl_multi_cleanup};
    QTimer m_performTimer{this};

    std::deque<Net::NetRequest::Ptr> m_pendingRequests{};
    std::vector<Net::NetRequest::Ptr> m_runningRequests{};
    std::deque<Net::NetRequest::Ptr> m_finishedRequests{};

    bool m_suppressSucceeded = false;

    int m_attempts = 1;
    int m_attemptsBeforeAsking = 3;
    bool m_askRetry = true;
    int m_manualRetries = 0;
};
