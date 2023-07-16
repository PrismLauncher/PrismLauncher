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

#include <QNetworkReply>
#include <QUrl>

#include "QObjectPtr.h"
#include "tasks/Task.h"

class NetAction : public Task {
    Q_OBJECT
   protected:
    explicit NetAction() : Task(){};

   public:
    using Ptr = shared_qobject_ptr<NetAction>;

    virtual ~NetAction() = default;

    QUrl url() { return m_url; }

    void setNetwork(shared_qobject_ptr<QNetworkAccessManager> network) { m_network = network; }

   protected slots:
    virtual void downloadProgress(qint64 bytesReceived, qint64 bytesTotal) = 0;
    virtual void downloadError(QNetworkReply::NetworkError error) = 0;
    virtual void downloadFinished() = 0;
    virtual void downloadReadyRead() = 0;

    virtual void sslErrors(const QList<QSslError>& errors) {
        int i = 1;
        for (auto error : errors) {
            qCritical() << "Network SSL Error #" << i << " : " << error.errorString();
            auto cert = error.certificate();
            qCritical() << "Certificate in question:\n" << cert.toText();
            i++;
        }

    };

   public slots:
    void startAction(shared_qobject_ptr<QNetworkAccessManager> network)
    {
        m_network = network;
        executeTask();
    }

   protected:
    void executeTask() override{};

   public:
    shared_qobject_ptr<QNetworkAccessManager> m_network;

    /// the network reply
    unique_qobject_ptr<QNetworkReply> m_reply;

    /// source URL
    QUrl m_url;
};
