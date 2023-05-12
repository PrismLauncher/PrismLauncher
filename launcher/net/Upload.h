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

#pragma once

#include "NetAction.h"
#include "Sink.h"

namespace Net {

    class Upload : public NetAction {
        Q_OBJECT

    public:
        using Ptr = shared_qobject_ptr<Upload>;

        static Upload::Ptr makeByteArray(QUrl url, QByteArray *output, QByteArray m_post_data);
        auto abort() -> bool override;
        auto canAbort() const -> bool override { return true; };

    protected slots:
        void downloadProgress(qint64 bytesReceived, qint64 bytesTotal) override;
        void downloadError(QNetworkReply::NetworkError error) override;
        void sslErrors(const QList<QSslError> & errors) override;
        void downloadFinished() override;
        void downloadReadyRead() override;

    public slots:
        void executeTask() override;
    private:
        std::unique_ptr<Sink> m_sink;
        QByteArray m_post_data;

        bool handleRedirect();
    };

} // Net

