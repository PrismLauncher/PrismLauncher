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

#include <QObject>
#include <QString>
#include <QVariantMap>

class LauncherSettings;
class NetworkContext;

/**
 * Delivers OAuth redirects that arrive through the launcher's custom URL scheme
 * (forwarded from the running instance) to the MSA login step.
 */
class OAuthCallbackRelay : public QObject {
    Q_OBJECT

   public:
    using QObject::QObject;

   signals:
    void replyReceived(QVariantMap reply);
};

/**
 * Dependencies of the Microsoft auth step pipeline.
 *
 * Application builds one from its owned services; AccountList and the UI pass it
 * to MinecraftAccount::login()/refresh(). All pointers are non-owning and must
 * outlive any AuthFlow built from this context.
 */
struct AuthContext {
    NetworkContext* network = nullptr;
    /** Source of MSAClientIDOverride; null means the build-time client ID is used. */
    LauncherSettings* settings = nullptr;
    /** Null forces MSA login to use the loopback HTTP redirect instead of the custom URL scheme. */
    OAuthCallbackRelay* oauthCallbacks = nullptr;
    /** Portable installs cannot rely on a registered URL scheme handler. */
    bool portable = false;
    /** Desktop entry name used to check the URL scheme handler on Linux. */
    QString desktopFileName;

    /** MSAClientIDOverride when set, else the build-time MSA client ID. */
    QString msaClientId() const;
};
