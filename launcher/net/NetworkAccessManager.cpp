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

#include "NetworkAccessManager.h"

#include <QDateTime>
#include <QHostInfo>
#include <QUrl>

namespace Net {

namespace {
constexpr qint64 dnsCacheTtlSecs = 300;
}

QHostAddress NetworkAccessManager::resolveIPv4(const QString& hostName)
{
    const qint64 now = QDateTime::currentSecsSinceEpoch();

    auto it = m_dnsCache.find(hostName);
    if (it != m_dnsCache.end()) {
        if (it->expiresAt > now)
            return it->address;
        m_dnsCache.erase(it);
    }

    const QHostInfo info = QHostInfo::fromName(hostName);
    for (const auto& addr : info.addresses()) {
        if (addr.protocol() == QAbstractSocket::IPv4Protocol) {
            m_dnsCache.insert(hostName, { addr, now + dnsCacheTtlSecs });
            return addr;
        }
    }
    // no IPv4 address available for this host; caller falls back to default behavior
    return {};
}

QNetworkReply* NetworkAccessManager::createRequest(Operation op, const QNetworkRequest& originalReq, QIODevice* outgoingData)
{
    QUrl url = originalReq.url();
    const QString scheme = url.scheme();
    const QString host = url.host();

    // Only rewrite plain hostnames on http(s); leave literal IPs and other schemes untouched.
    if ((scheme == QLatin1String("http") || scheme == QLatin1String("https")) && !host.isEmpty() && QHostAddress(host).isNull()) {
        const QHostAddress ipv4 = resolveIPv4(host);
        if (!ipv4.isNull()) {
            QNetworkRequest req = originalReq;
            url.setHost(ipv4.toString());
            req.setUrl(url);
            req.setRawHeader("Host", host.toUtf8());
            req.setPeerVerifyName(host);
            // HTTP/2 sends ":authority" from the request URL (the IP literal here), ignoring
            // the raw Host header above, which servers doing name-based routing reject. Force
            // HTTP/1.1 so the Host header override actually takes effect.
            req.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
            return QNetworkAccessManager::createRequest(op, req, outgoingData);
        }
    }

    return QNetworkAccessManager::createRequest(op, originalReq, outgoingData);
}

}  // namespace Net
