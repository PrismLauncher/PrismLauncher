// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
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

#include <QtNetwork>

#include <QObject>
#include "NetAction.h"
#include "tasks/Task.h"

// Those are included so that they are also included by anyone using NetJob
#include "net/Download.h"
#include "net/HttpMetaCache.h"

class NetJob : public Task {
    Q_OBJECT

   public:
    using Ptr = shared_qobject_ptr<NetJob>;

    explicit NetJob(QString job_name, shared_qobject_ptr<QNetworkAccessManager> network) : Task(), m_network(network)
    {
        setObjectName(job_name);
    }
    virtual ~NetJob() = default;

    void executeTask() override;

    auto canAbort() const -> bool override;

    auto addNetAction(NetAction::Ptr action) -> bool;

    auto operator[](int index) -> NetAction::Ptr { return m_downloads[index]; }
    auto at(int index) -> const NetAction::Ptr { return m_downloads.at(index); }
    auto size() const -> int { return m_downloads.size(); }
    auto first() -> NetAction::Ptr { return m_downloads.size() != 0 ? m_downloads[0] : NetAction::Ptr{}; }

    auto getFailedFiles() -> QStringList;

   public slots:
    // Qt can't handle auto at the start for some reason?
    bool abort() override;

   private slots:
    void startMoreParts();

    void partProgress(int index, qint64 bytesReceived, qint64 bytesTotal);
    void partSucceeded(int index);
    void partFailed(int index);
    void partAborted(int index);

   private:
    shared_qobject_ptr<QNetworkAccessManager> m_network;

    struct part_info {
        qint64 current_progress = 0;
        qint64 total_progress = 1;
        int failures = 0;
    };

    QList<NetAction::Ptr> m_downloads;
    QList<part_info> m_parts_progress;
    QQueue<int> m_todo;
    QSet<int> m_doing;
    QSet<int> m_done;
    QSet<int> m_failed;
    qint64 m_current_progress = 0;
    bool m_aborted = false;
};
