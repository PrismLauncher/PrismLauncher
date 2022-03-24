// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#pragma once
#include <QString>
#include <QByteArray>
#include <QVector>
#include <katabasis/Bits.h>
#include <QJsonObject>

struct Skin {
    QString id;
    QString url;
    QString variant;

    QByteArray data;
};

struct Cape {
    QString id;
    QString url;
    QString alias;

    QByteArray data;
};

struct MinecraftEntitlement {
    bool ownsMinecraft = false;
    bool canPlayMinecraft = false;
    Katabasis::Validity validity = Katabasis::Validity::None;
};

struct MinecraftProfile {
    QString id;
    QString name;
    Skin skin;
    QString currentCape;
    QMap<QString, Cape> capes;
    Katabasis::Validity validity = Katabasis::Validity::None;
};

enum class AccountType {
    MSA,
    Mojang,
    Offline
};

enum class AccountState {
    Unchecked,
    Offline,
    Working,
    Online,
    Disabled,
    Errored,
    Expired,
    Gone
};

struct AccountData {
    QJsonObject saveState() const;
    bool resumeStateFromV2(QJsonObject data);
    bool resumeStateFromV3(QJsonObject data);

    //! userName for Mojang accounts, gamertag for MSA
    QString accountDisplayString() const;

    //! Only valid for Mojang accounts. MSA does not preserve this information
    QString userName() const;

    //! Only valid for Mojang accounts.
    QString clientToken() const;
    void setClientToken(QString clientToken);
    void invalidateClientToken();
    void generateClientTokenIfMissing();

    //! Yggdrasil access token, as passed to the game.
    QString accessToken() const;

    QString profileId() const;
    QString profileName() const;

    QString lastError() const;

    AccountType type = AccountType::MSA;
    bool legacy = false;
    bool canMigrateToMSA = false;

    QString msaClientID;
    Katabasis::Token msaToken;
    Katabasis::Token userToken;
    Katabasis::Token xboxApiToken;
    Katabasis::Token mojangservicesToken;

    Katabasis::Token yggdrasilToken;
    MinecraftProfile minecraftProfile;
    MinecraftEntitlement minecraftEntitlement;
    Katabasis::Validity validity_ = Katabasis::Validity::None;

    // runtime only information (not saved with the account)
    QString internalId;
    QString errorString;
    AccountState accountState = AccountState::Unchecked;
};
