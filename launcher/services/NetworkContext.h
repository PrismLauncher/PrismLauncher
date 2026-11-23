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

#include <QString>

class HttpMetaCache;
class QNetworkAccessManager;

/**
 * Narrow network contract: shared QNAM, HTTP meta-cache, user-agent, and
 * provider API credentials.
 *
 * Does not own the network stack; Application does. Inject this into NetJob
 * creators and API clients instead of reading APPLICATION->network().
 */
class NetworkContext {
   public:
    virtual ~NetworkContext() = default;

    virtual QNetworkAccessManager* qnam() = 0;
    virtual HttpMetaCache* httpMetaCache() = 0;
    virtual QString userAgent() const = 0;

    /** CurseForge API key (user override, else build-time key). Empty disables Flame. */
    virtual QString flameApiKey() const = 0;
    /** Modrinth personal access token, or a null string when none is configured. */
    virtual QString modrinthApiToken() const = 0;
};
