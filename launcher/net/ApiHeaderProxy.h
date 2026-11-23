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

#include "BuildConfig.h"
#include "net/HeaderProxy.h"
#include "services/NetworkContext.h"

#include <QJsonDocument>
#include <QJsonObject>

namespace Net {

struct ModrinthDownloadMeta {
    QString reason;
    QString gameVersion;
    QString loader;
    QString dependentOn;

    bool isEmpty() const { return reason.isEmpty(); }

    QByteArray toJson() const
    {
        QJsonObject obj;
        if (!reason.isEmpty()) {
            obj["reason"] = reason;
        }
        if (!gameVersion.isEmpty()) {
            obj["game_version"] = gameVersion;
        }
        if (!loader.isEmpty()) {
            obj["loader"] = loader;
        }
        if (!dependentOn.isEmpty()) {
            obj["dependent_on"] = dependentOn;
        }
        return QJsonDocument(obj).toJson(QJsonDocument::Compact);
    }
};

/**
 * Adds provider credentials (CurseForge key, Modrinth token) and Modrinth download metadata.
 *
 * A null NetworkContext adds only the download metadata header.
 */
class ApiHeaderProxy : public HeaderProxy {
   public:
    explicit ApiHeaderProxy(const NetworkContext* network, ModrinthDownloadMeta meta = ModrinthDownloadMeta())
        : m_network(network), m_meta(std::move(meta))
    {}
    ~ApiHeaderProxy() override = default;

   public:
    QList<HeaderPair> headers(const QNetworkRequest& request) const override
    {
        QList<HeaderPair> hdrs;
        const auto host = request.url().host();

        if (m_network != nullptr) {
            if (host == QUrl(BuildConfig.FLAME_BASE_URL).host() || host == BuildConfig.FLAME_DOWNLOAD_HOST) {
                QString key = m_network->flameApiKey();
                if (!key.isEmpty()) {
                    hdrs.append({ .headerName = "x-api-key", .headerValue = key.toUtf8() });
                }
            } else if (host == QUrl(BuildConfig.MODRINTH_PROD_URL).host() || host == QUrl(BuildConfig.MODRINTH_STAGING_URL).host()) {
                QString token = m_network->modrinthApiToken();
                if (!token.isNull()) {
                    hdrs.append({ .headerName = "Authorization", .headerValue = token.toUtf8() });
                }
            }
        }

        if (host == BuildConfig.MODRINTH_DOWNLOAD_HOST && !m_meta.isEmpty()) {
            hdrs.append({ .headerName = "modrinth-download-meta", .headerValue = m_meta.toJson() });
        }
        return hdrs;
    };

   private:
    const NetworkContext* m_network = nullptr;
    ModrinthDownloadMeta m_meta;
};

}  // namespace Net
