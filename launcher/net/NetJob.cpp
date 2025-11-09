// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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

#include "NetJob.h"

#include <ui/dialogs/NetworkJobFailedDialog.h>

#include <QNetworkReply>
#include "net/NetRequest.h"
#include "tasks/ConcurrentTask.h"
#if defined(LAUNCHER_APPLICATION)
#include "Application.h"
#include "ui/dialogs/CustomMessageBox.h"
#endif

NetJob::NetJob(QString jobName, int maxConcurrent)
{
    if (maxConcurrent < 0) {
#if defined(LAUNCHER_APPLICATION)
        maxConcurrent = APPLICATION_DYN ? APPLICATION->settings()->get("NumberOfConcurrentDownloads").toInt() : 6;
#else
        maxConcurrent = 6;
#endif
    }

    m_jobName = jobName;
    m_maxConcurrent = maxConcurrent;
    m_runningRequests.reserve(maxConcurrent);

    connect(this, &Task::finished, this, [this]{ m_performTimer.stop(); });
}

NetJob::~NetJob()
{
    for (const auto& request : m_runningRequests) {
        curl_multi_remove_handle(m_curl.get(), request->curlHandle());
    }
}

auto NetJob::addNetAction(Net::NetRequest::Ptr action) -> void
{
    m_pendingRequests.push_back(action);
}

size_t NetJob::requestsSize() const
{
    return m_pendingRequests.size() + m_runningRequests.size() + m_failedRequests.size();
}

std::deque<Net::NetRequest::Ptr> NetJob::getFailedRequests() const
{
    return m_failedRequests;
}

void NetJob::executeTask()
{
    connect(&m_performTimer, &QTimer::timeout, this, &NetJob::perform);
    m_performTimer.start();
}

void NetJob::perform()
{
    int runningTasks;
    if (const CURLMcode result = curl_multi_perform(m_curl.get(), &runningTasks); result != CURLM_OK) {
        qCritical() << "curl_multi_perform failed:" << curl_multi_strerror(result);
        m_performTimer.stop();

        Task::emitFailed("curl_multi_perform failed");
        return;
    }

    while (runningTasks < m_maxConcurrent) {
        if (m_pendingRequests.empty()) {
            break;
        }
        auto request = m_pendingRequests.front();
        m_pendingRequests.pop_front();

        if (const auto state = request->prepare(); state == State::Running) {
            curl_multi_add_handle(m_curl.get(), request->curlHandle());
            m_runningRequests.push_back(request);
            runningTasks++;
        }
    }

    qint64 totalExpected = 0;
    qint64 totalReceived = 0;
    for (const auto& request : m_runningRequests) {
        auto taskProgress = request->stepProgress();

        totalExpected += taskProgress.total;
        totalReceived += taskProgress.current;

        emit stepProgress(taskProgress);
        request->updateDetails();
    }

    emit progress(totalReceived, totalExpected);

    const CURLMsg* multiMsg = nullptr;
    do {
        int messagesInQueue;
        multiMsg = curl_multi_info_read(m_curl.get(), &messagesInQueue);
        if (multiMsg && multiMsg->msg == CURLMSG_DONE) {
            const auto& request = findRequestByHandle(multiMsg->easy_handle);
            const CURLcode result = multiMsg->data.result;
            request->setResult(result);

            curl_multi_remove_handle(m_curl.get(), multiMsg->easy_handle);
            std::erase(m_runningRequests, request);

            if (request->isSuccess()) {
                request->finalize();
                emit request->succeeded();
            } else {
                m_failedRequests.push_back(request);
            }
        }
    } while (multiMsg);

    if (runningTasks == 0) {
        onAllTransfersComplete();
    }
}

Net::NetRequest::Ptr& NetJob::findRequestByHandle(const CURL* handle)
{
    for (auto& request : m_runningRequests) {
        if (request->curlHandle() == handle) {
            return request;
        }
    }

    qCritical() << "No request found for handle";
    throw std::invalid_argument{ "No request found for handle" };
}

void NetJob::onAllTransfersComplete()
{
    if (!isRunning()) {
        m_performTimer.stop();
        return;
    }

    const bool success = m_failedRequests.empty();
    bool shouldStop = true;

    if (m_attempts < m_attemptsBeforeAsking && isOnline()) {
        while (!m_failedRequests.empty()) {
            auto request = m_failedRequests.front();
            m_failedRequests.pop_front();
            m_pendingRequests.push_back(request);
            shouldStop = false;
        }
    }

    if (!shouldStop) {
        if (!success) {
            m_attempts++;
        }
        return;
    }

    if (success) {
        if (!m_suppressSucceeded) {
            emitSucceeded();
        }
    } else {
        emitFailed("Network requests have failed");
    }
}

auto NetJob::abort() -> bool
{
    for (const auto& request : m_pendingRequests) {
        request->abort();
    }
    for (const auto& request : m_runningRequests) {
        request->abort();
        curl_multi_remove_handle(m_curl.get(), request->curlHandle());
    }

    m_performTimer.stop();
    emitAborted();
    return true;
}

bool NetJob::isOnline() const
{
    // check some errors that are ussually associated with the lack of internet
    for (const auto& request : m_failedRequests) {
        const auto result = request->result();
        if (result != CURLE_COULDNT_RESOLVE_HOST) {
            return true;
        }
    }
    return false;
}

void NetJob::emitFailed(QString reason)
{
#if defined(LAUNCHER_APPLICATION)
    if (APPLICATION_DYN && m_askRetry && m_manualRetries < APPLICATION->settings()->get("NumberOfManualRetries").toInt() && isOnline()) {
        auto dialog = NetworkJobFailedDialog(m_jobName, m_attempts, m_failedRequests.size(), m_failedRequests.size(), nullptr);

        int i = 0;
        for (const auto& request : m_failedRequests) {
            dialog.addFailedRequest(i, request->url(), request->error());
            i++;
        }

        if (dialog.exec() == QDialog::Accepted) {
            m_manualRetries++;
            m_attemptsBeforeAsking += 3;
            return;
        }
    }
#endif

    for (const auto& request : m_failedRequests) {
        emit request->failed(request->error());
    }

    Task::emitFailed(reason);
}

void NetJob::setAskRetry(bool askRetry)
{
    m_askRetry = askRetry;
}

void NetJob::setSuppressSucceeded(bool suppressSucceeded)
{
    m_suppressSucceeded = suppressSucceeded;
}

bool NetJob::isMultiStep() const
{
    return requestsSize() > 1;
}
