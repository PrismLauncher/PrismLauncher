// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2025 PineconeMC Contributors
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

#pragma once

#include <QFile>
#include <QNetworkReply>
#include <QUrl>

#include "Sink.h"

class DownloadCache;

namespace Net {

class ResumingFileSink : public Sink {
   public:
    ResumingFileSink(QString finalPath, QUrl url, ::DownloadCache* cache = nullptr);
    virtual ~ResumingFileSink() = default;

    auto init(QNetworkRequest& request) -> Task::State override;
    auto write(QByteArray& data) -> Task::State override;
    auto abort() -> Task::State override;
    auto finalize(QNetworkReply& reply) -> Task::State override;
    auto hasLocalData() -> bool override;

   private:
    QString m_finalPath;
    QUrl m_url;
    ::DownloadCache* m_cache;
    qint64 m_existingSize = 0;
    bool m_usedRange = false;
    bool m_wroteAnyData = false;
    std::unique_ptr<QFile> m_partFile;
};

}  // namespace Net
