// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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
 */

#pragma once

#include "net/HeaderProxy.h"

namespace Net {

class StaticHeaderProxy : public HeaderProxy {
   public:
    StaticHeaderProxy(QList<HeaderPair> hdrs = {}) : HeaderProxy(), m_hdrs(hdrs){};
    virtual ~StaticHeaderProxy() = default;

   public:
    virtual QList<HeaderPair> headers(const QNetworkRequest&) const override { return m_hdrs; };
    void setHeaders(QList<HeaderPair> hdrs) { m_hdrs = hdrs; };

   private:
    QList<HeaderPair> m_hdrs;
};

}  // namespace Net
