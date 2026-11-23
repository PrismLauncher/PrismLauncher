// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Prism Launcher Contributors
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

#include "services/NetworkContext.h"

class SettingsObject;

/**
 * NetworkContext backed by Application-owned QNAM / HttpMetaCache and settings
 * used to resolve the effective user-agent string.
 */
class SharedNetworkContext : public NetworkContext {
   public:
    SharedNetworkContext(QNetworkAccessManager* nam, HttpMetaCache* metaCache, SettingsObject* settings);
    ~SharedNetworkContext() override = default;

    QNetworkAccessManager* qnam() override;
    HttpMetaCache* httpMetaCache() override;
    QString userAgent() const override;
    QString flameApiKey() const override;
    QString modrinthApiToken() const override;

   private:
    QNetworkAccessManager* m_nam = nullptr;
    HttpMetaCache* m_metaCache = nullptr;
    SettingsObject* m_settings = nullptr;
};
