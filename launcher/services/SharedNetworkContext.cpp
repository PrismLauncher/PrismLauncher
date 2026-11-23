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

#include "services/SharedNetworkContext.h"

#include "BuildConfig.h"
#include "settings/SettingsObject.h"

SharedNetworkContext::SharedNetworkContext(QNetworkAccessManager* nam, HttpMetaCache* metaCache, SettingsObject* settings)
    : m_nam(nam), m_metaCache(metaCache), m_settings(settings)
{}

QNetworkAccessManager* SharedNetworkContext::qnam()
{
    return m_nam;
}

HttpMetaCache* SharedNetworkContext::httpMetaCache()
{
    return m_metaCache;
}

QString SharedNetworkContext::userAgent() const
{
    QString uaOverride = m_settings->get("UserAgentOverride").toString();
    if (!uaOverride.isEmpty()) {
        return uaOverride.replace("$LAUNCHER_VER", BuildConfig.printableVersionString());
    }
    return BuildConfig.USER_AGENT;
}

QString SharedNetworkContext::flameApiKey() const
{
    QString keyOverride = m_settings->get("FlameKeyOverride").toString();
    if (!keyOverride.isEmpty()) {
        return keyOverride;
    }
    return BuildConfig.FLAME_API_KEY;
}

QString SharedNetworkContext::modrinthApiToken() const
{
    QString tokenOverride = m_settings->get("ModrinthToken").toString();
    if (!tokenOverride.isEmpty()) {
        return tokenOverride;
    }
    return {};
}
