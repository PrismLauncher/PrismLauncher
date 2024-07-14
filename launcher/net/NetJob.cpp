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
#include "net/NetRequest.h"
#include "tasks/ConcurrentTask.h"
#if defined(LAUNCHER_APPLICATION)
#include "Application.h"
#endif

NetJob::NetJob(QString job_name, shared_qobject_ptr<QNetworkAccessManager> network, int max_concurrent)
    : ConcurrentTask(nullptr, job_name), m_network(network)
{
#if defined(LAUNCHER_APPLICATION)
    if (max_concurrent < 0)
        max_concurrent = APPLICATION->settings()->get("NumberOfConcurrentDownloads").toInt();
#endif
    if (max_concurrent > 0)
        setMaxConcurrent(max_concurrent);
}

void NetJob::addNetAction(Net::NetRequest::Ptr action)
{
    action->setNetwork(m_network);
    addTask(action);
}

QList<Net::NetRequest*> NetJob::getFailedActions()
{
    QList<Net::NetRequest*> failed;
    for (auto index : m_failed) {
        failed.push_back(dynamic_cast<Net::NetRequest*>(index.get()));
    }
    return failed;
}

QList<QString> NetJob::getFailedFiles()
{
    QList<QString> failed;
    for (auto index : m_failed) {
        failed.append(static_cast<Net::NetRequest*>(index.get())->url().toString());
    }
    return failed;
}
