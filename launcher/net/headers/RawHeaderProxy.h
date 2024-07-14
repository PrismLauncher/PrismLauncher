// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2023 Rachel Powers <508861+Ryex@users.noreply.github.com>
 *  Copyright (c) 2023 Trial97 <alexandru.tripon97@gmail.com>
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
 */

#pragma once

#include "HeaderProxy.h"

namespace Net {

struct HeaderPair {
    QByteArray headerName;
    QByteArray headerValue;
};

class RawHeaderProxy : public HeaderProxy {
   public:
    RawHeaderProxy(QList<HeaderPair> headers = {}) : HeaderProxy(), m_headers(headers) {};
    virtual ~RawHeaderProxy() = default;

   public:
    virtual void writeHeaders(QNetworkRequest& request) override
    {
        for (auto hdr : m_headers) {
            request.setRawHeader(hdr.headerName, hdr.headerValue);
        }
    }

    void addHeader(const HeaderPair& header) { m_headers.append(header); }
    void addHeader(const QByteArray& headerName, const QByteArray& headerValue) { m_headers.append({ headerName, headerValue }); }
    void addHeaders(const QList<HeaderPair>& headers) { m_headers.append(headers); }
    void setHeaders(QList<HeaderPair> headers) { m_headers = headers; };

   private:
    QList<HeaderPair> m_headers;
};

}  // namespace Net
