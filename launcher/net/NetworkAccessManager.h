// SPDX-License-Identifier: GPL-3.0-only
/*
 * Prism Launcher - Minecraft Launcher
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <QHash>
#include <QHostAddress>
#include <QNetworkAccessManager>

namespace Net {

/// QNetworkAccessManager that forces outgoing HTTP(S) requests over IPv4.
///
/// Qt has no public API to restrict QNetworkAccessManager to a single network
/// layer protocol. Some dual-stack networks silently fail IPv6 connections to
/// Xbox Live and libraries.minecraft.net, so we resolve the request's hostname
/// ourselves and connect to its IPv4 address directly, while keeping the
/// original hostname for the HTTP Host header and TLS SNI/certificate
/// validation (via QNetworkRequest::setPeerVerifyName).
class NetworkAccessManager : public QNetworkAccessManager {
    Q_OBJECT
   public:
    using QNetworkAccessManager::QNetworkAccessManager;

   protected:
    QNetworkReply* createRequest(Operation op, const QNetworkRequest& originalReq, QIODevice* outgoingData) override;

   private:
    QHostAddress resolveIPv4(const QString& hostName);

    struct CacheEntry {
        QHostAddress address;
        qint64 expiresAt;
    };
    QHash<QString, CacheEntry> m_dnsCache;
};

}  // namespace Net
