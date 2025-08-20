// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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

#include "AccountData.h"
#include "BuildConfig.h"

#include <qt6keychain/keychain.h>
#include <QDebug>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

namespace {
QJsonValue saveTokenV3(const Token& token)
{
    if (!token.persistent)
        return QJsonValue(QJsonValue::Undefined);

    QJsonObject out;

    bool save = false;

    if (!token.token.isEmpty()) {
        out["token"] = QJsonValue(token.token);
        save = true;
    }

    if (!token.refresh_token.isEmpty()) {
        out["refresh_token"] = QJsonValue(token.refresh_token);
        save = true;
    }

    if (!token.extra.isEmpty()) {
        out["extra"] = QJsonObject::fromVariantMap(token.extra);
        save = true;
    }

    if (!save)
        return QJsonValue(QJsonValue::Undefined);

    if (token.issueInstant.isValid())
        out["iat"] = QJsonValue(token.issueInstant.toSecsSinceEpoch());

    if (token.notAfter.isValid())
        out["exp"] = QJsonValue(token.notAfter.toSecsSinceEpoch());

    return out;
}

Token loadTokenJSONV3(const QJsonObject& obj)
{
    Token out;

    if (obj.isEmpty())
        return out;

    QJsonValue issueInstant = obj.value("iat");
    if (issueInstant.isDouble())
        out.issueInstant = QDateTime::fromSecsSinceEpoch((int64_t)issueInstant.toDouble());

    QJsonValue notAfter = obj.value("exp");
    if (notAfter.isDouble())
        out.notAfter = QDateTime::fromSecsSinceEpoch((int64_t)notAfter.toDouble());

    QJsonValue token = obj.value("token");
    if (token.isString()) {
        out.token = token.toString();
        out.validity = Validity::Assumed;
    }

    QJsonValue refresh_token = obj.value("refresh_token");
    if (refresh_token.isString())
        out.refresh_token = refresh_token.toString();

    QJsonValue extra = obj.value("extra");
    if (extra.isObject())
        out.extra = extra.toObject().toVariantMap();

    return out;
}

QJsonValue saveSkinV3(const Skin& skin)
{
    QJsonObject out;
    out["id"] = skin.id;
    out["url"] = skin.url;
    out["variant"] = skin.variant;

    if (!skin.data.isEmpty())
        out["data"] = QString::fromLatin1(skin.data.toBase64());

    return out;
}

Skin loadSkinV3(const QJsonObject& obj)
{
    Skin out;

    out.id = obj.value("id").toString();
    out.url = obj.value("url").toString();
    out.variant = obj.value("variant").toString();

    QJsonValue data = obj.value("data");

    if (out.id.isNull() || out.url.isNull() || out.variant.isNull()) {
        qWarning() << "Skin must contain strings id, url, variant";
        return out;
    }

    if (!data.isUndefined()) {
        if (!data.isString()) {
            qWarning() << "Skin data must be a string";
            return out;
        }

        out.data = QByteArray::fromBase64(data.toString().toLatin1());

        if (out.data.isEmpty()) {
            qWarning() << "Skin data is invalid";
            return out;
        }
    }

    return out;
}

QJsonValue saveCapeV3(const Cape& cape)
{
    QJsonObject out;
    out["id"] = cape.id;
    out["url"] = cape.url;
    out["alias"] = cape.alias;

    if (!cape.data.isEmpty())
        out["data"] = QString::fromLatin1(cape.data.toBase64());

    return out;
}

Cape loadCapeV3(const QJsonObject& obj)
{
    Cape out;

    out.id = obj.value("id").toString();
    out.url = obj.value("url").toString();
    out.url.replace("http://textures.minecraft.net", "https://textures.minecraft.net");
    out.alias = obj.value("alias").toString();

    QJsonValue data = obj.value("data");

    if (out.id.isNull() || out.url.isNull() || out.alias.isNull()) {
        qWarning() << "Cape must contain strings id, url, alias";
        return out;
    }

    if (!data.isUndefined()) {
        if (!data.isString()) {
            qWarning() << "Cape data must be a string";
            return out;
        }

        out.data = QByteArray::fromBase64(data.toString().toLatin1());

        if (out.data.isEmpty()) {
            qWarning() << "Cape data is invalid";
            return out;
        }
    }

    return out;
}

QJsonValue saveProfileV3(const MinecraftProfile& profile)
{
    if (profile.id.isEmpty())
        return QJsonValue(QJsonValue::Undefined);

    QJsonObject out;
    out["id"] = QJsonValue(profile.id);
    out["name"] = QJsonValue(profile.name);

    if (!profile.currentCape.isEmpty())
        out["cape"] = profile.currentCape;

    out["skin"] = saveSkinV3(profile.skin);

    QJsonArray capesArray;
    for (const Cape& cape : profile.capes)
        capesArray.push_back(saveCapeV3(cape));

    out["capes"] = capesArray;

    return out;
}

MinecraftProfile loadProfileV3(const QJsonObject& obj)
{
    MinecraftProfile out;

    if (obj.isEmpty())
        return out;

    out.id = obj.value("id").toString();
    out.name = obj.value("name").toString();

    if (out.id.isNull() || out.name.isNull()) {
        qWarning() << "Profile must contain strings id, name";
        return MinecraftProfile();
    }

    out.skin = loadSkinV3(obj.value("skin").toObject());

    auto capes = obj.value("capes");

    if (capes.isArray()) {
        for (QJsonValue capeObj : capes.toArray()) {
            Cape cape = loadCapeV3(capeObj.toObject());
            out.capes[cape.id] = std::move(cape);
        }
    } else
        qWarning() << "Profile capes must be an array";

    QString currentCape = obj["cape"].toString();
    if (!currentCape.isEmpty() || out.capes.contains(currentCape))
        out.currentCape = currentCape;

    out.validity = Validity::Assumed;
    return out;
}

QJsonValue saveEntitlementV3(MinecraftEntitlement entitlement)
{
    if (entitlement.validity == Validity::None)
        return QJsonValue(QJsonValue::Undefined);

    QJsonObject out;
    out["ownsMinecraft"] = entitlement.ownsMinecraft;
    out["canPlayMinecraft"] = entitlement.canPlayMinecraft;

    return out;
}

MinecraftEntitlement loadEntitlementV3(const QJsonObject& obj)
{
    MinecraftEntitlement out;

    if (obj.isEmpty())
        return out;

    QJsonValue ownsMinecraft = obj.value("ownsMinecraft");
    QJsonValue canPlayMinecraft = obj.value("canPlayMinecraft");

    if (!ownsMinecraft.isBool() || !canPlayMinecraft.isBool()) {
        qWarning() << "Entitlement must contain booleans ownsMinecraft, canPlayMinecraft";
        return out;
    }

    out.canPlayMinecraft = canPlayMinecraft.toBool(false);
    out.ownsMinecraft = ownsMinecraft.toBool(false);
    out.validity = Validity::Assumed;

    return out;
}

}  // namespace

bool AccountData::resumeStateFromV3(QJsonObject data)
{
    auto typeV = data.value("type");
    if (!typeV.isString()) {
        qWarning() << "Failed to parse account data: type is missing.";
        return false;
    }
    auto typeS = typeV.toString();
    if (typeS == "MSA") {
        type = AccountType::MSA;
    } else if (typeS == "Offline") {
        type = AccountType::Offline;
    } else {
        qWarning() << "Failed to parse account data: type is not recognized.";
        return false;
    }

    if (type == AccountType::MSA) {
        auto clientIDV = data.value("msa-client-id");
        if (clientIDV.isString()) {
            msaClientID = clientIDV.toString();
        }  // leave msaClientID empty if it doesn't exist or isn't a string
        msaToken = loadTokenJSONV3(data["msa"].toObject());
        userToken = loadTokenJSONV3(data["utoken"].toObject());
        xboxApiToken = loadTokenJSONV3(data["xrp-main"].toObject());
        mojangservicesToken = loadTokenJSONV3(data["xrp-mc"].toObject());
    }

    yggdrasilToken = loadTokenJSONV3(data["ygg"].toObject());
    // versions before 7.2 used "offline" as the offline token
    if (yggdrasilToken.token == "offline")
        yggdrasilToken.token = "0";

    minecraftProfile = loadProfileV3(data["profile"].toObject());
    minecraftEntitlement = loadEntitlementV3(data["entitlement"].toObject());

    validity_ = minecraftProfile.validity;
    return true;
}

QJsonObject AccountData::saveState() const
{
    QJsonObject output;
    if (type == AccountType::MSA) {
        output["type"] = "MSA";
        output["msa-client-id"] = msaClientID;
        output["msa"] = saveTokenV3(msaToken);
        output["utoken"] = saveTokenV3(userToken);
        output["xrp-main"] = saveTokenV3(xboxApiToken);
        output["xrp-mc"] = saveTokenV3(mojangservicesToken);
    } else if (type == AccountType::Offline) {
        output["type"] = "Offline";
    }

    output["ygg"] = saveTokenV3(yggdrasilToken);
    output["profile"] = saveProfileV3(minecraftProfile);
    output["entitlement"] = saveEntitlementV3(minecraftEntitlement);

    return output;
}

QString AccountData::accessToken() const
{
    return yggdrasilToken.token;
}

QString AccountData::profileId() const
{
    return minecraftProfile.id;
}

QString AccountData::profileName() const
{
    if (minecraftProfile.name.size() == 0) {
        return QObject::tr("No profile (%1)").arg(accountDisplayString());
    } else {
        return minecraftProfile.name;
    }
}

QString AccountData::accountDisplayString() const
{
    switch (type) {
        case AccountType::Offline: {
            return QObject::tr("<Offline>");
        }
        case AccountType::MSA: {
            if (xboxApiToken.extra.contains("gtg")) {
                return xboxApiToken.extra["gtg"].toString();
            }
            return "Xbox profile missing";
        }
        default: {
            return "Invalid Account";
        }
    }
}

QString AccountData::lastError() const
{
    return errorString;
}
