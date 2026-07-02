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

#include "Application.h"
#include "BuildConfig.h"
#include "net/HeaderProxy.h"

#include <QJsonDocument>
#include <QJsonObject>

namespace Net {

struct ModrinthDownloadMeta {
    QString reason;
    QString gameVersion;
    QString loader;

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
        return QJsonDocument(obj).toJson(QJsonDocument::Compact);
    }
};

class ApiHeaderProxy : public HeaderProxy {
   public:
    ApiHeaderProxy() = default;
    explicit ApiHeaderProxy(ModrinthDownloadMeta meta) : m_meta(std::move(meta)) {}
    ~ApiHeaderProxy() override = default;

   public:
    QList<HeaderPair> headers(const QNetworkRequest& request) const override
    {
        QList<HeaderPair> hdrs;
        const auto host = request.url().host();

        if (APPLICATION->capabilities() & Application::SupportsFlame &&
            (host == QUrl(BuildConfig.FLAME_BASE_URL).host() || host == BuildConfig.FLAME_DOWNLOAD_HOST)) {
            hdrs.append({ .headerName = "x-api-key", .headerValue = APPLICATION->getFlameAPIKey().toUtf8() });
        } else if (host == QUrl(BuildConfig.MODRINTH_PROD_URL).host() || host == QUrl(BuildConfig.MODRINTH_STAGING_URL).host()) {
            QString token = APPLICATION->getModrinthAPIToken();
            if (!token.isNull()) {
                hdrs.append({ .headerName = "Authorization", .headerValue = token.toUtf8() });
            }
        }

        if (host == BuildConfig.MODRINTH_DOWNLOAD_HOST && !m_meta.isEmpty()) {
            hdrs.append({ .headerName = "modrinth-download-meta", .headerValue = m_meta.toJson() });
        }
        return hdrs;
    };

   private:
    ModrinthDownloadMeta m_meta;
};

}  // namespace Net
