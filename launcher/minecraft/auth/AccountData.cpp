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
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUuid>

namespace {
void tokenToJSONV3(QJsonObject& parent, Token t, const char* tokenName)
{
    if (!t.persistent) {
        return;
    }
    QJsonObject out;
    if (t.issueInstant.isValid()) {
        out["iat"] = QJsonValue(t.issueInstant.toMSecsSinceEpoch() / 1000);
    }

    if (t.notAfter.isValid()) {
        out["exp"] = QJsonValue(t.notAfter.toMSecsSinceEpoch() / 1000);
    }

    bool save = false;
    if (!t.token.isEmpty()) {
        out["token"] = QJsonValue(t.token);
        save = true;
    }
    if (!t.refresh_token.isEmpty()) {
        out["refresh_token"] = QJsonValue(t.refresh_token);
        save = true;
    }
    if (t.extra.size()) {
        out["extra"] = QJsonObject::fromVariantMap(t.extra);
        save = true;
    }
    if (save) {
        parent[tokenName] = out;
    }
}

Token tokenFromJSONV3(const QJsonObject& parent, const char* tokenName)
{
    Token out;
    auto tokenObject = parent.value(tokenName).toObject();
    if (tokenObject.isEmpty()) {
        return out;
    }
    auto issueInstant = tokenObject.value("iat");
    if (issueInstant.isDouble()) {
        out.issueInstant = QDateTime::fromMSecsSinceEpoch(((int64_t)issueInstant.toDouble()) * 1000);
    }

    auto notAfter = tokenObject.value("exp");
    if (notAfter.isDouble()) {
        out.notAfter = QDateTime::fromMSecsSinceEpoch(((int64_t)notAfter.toDouble()) * 1000);
    }

    auto token = tokenObject.value("token");
    if (token.isString()) {
        out.token = token.toString();
        out.validity = Validity::Assumed;
    }

    auto refresh_token = tokenObject.value("refresh_token");
    if (refresh_token.isString()) {
        out.refresh_token = refresh_token.toString();
    }

    auto extra = tokenObject.value("extra");
    if (extra.isObject()) {
        out.extra = extra.toObject().toVariantMap();
    }
    return out;
}

void profileToJSONV3(QJsonObject& parent, MinecraftProfile p, const char* tokenName)
{
    if (p.id.isEmpty()) {
        return;
    }
    QJsonObject out;
    out["id"] = QJsonValue(p.id);
    out["name"] = QJsonValue(p.name);
    if (!p.currentCape.isEmpty()) {
        out["cape"] = p.currentCape;
    }

    {
        QJsonObject skinObj;
        skinObj["id"] = p.skin.id;
        skinObj["url"] = p.skin.url;
        skinObj["variant"] = p.skin.variant;
        if (p.skin.data.size()) {
            skinObj["data"] = QString::fromLatin1(p.skin.data.toBase64());
        }
        out["skin"] = skinObj;
    }

    QJsonArray capesArray;
    for (auto& cape : p.capes) {
        QJsonObject capeObj;
        capeObj["id"] = cape.id;
        capeObj["url"] = cape.url;
        capeObj["alias"] = cape.alias;
        if (cape.data.size()) {
            capeObj["data"] = QString::fromLatin1(cape.data.toBase64());
        }
        capesArray.push_back(capeObj);
    }
    out["capes"] = capesArray;
    parent[tokenName] = out;
}

MinecraftProfile profileFromJSONV3(const QJsonObject& parent, const char* tokenName)
{
    MinecraftProfile out;
    auto tokenObject = parent.value(tokenName).toObject();
    if (tokenObject.isEmpty()) {
        return out;
    }
    {
        auto idV = tokenObject.value("id");
        auto nameV = tokenObject.value("name");
        if (!idV.isString() || !nameV.isString()) {
            qWarning() << "mandatory profile attributes are missing or of unexpected type";
            return MinecraftProfile();
        }
        out.name = nameV.toString();
        out.id = idV.toString();
    }

    {
        auto skinV = tokenObject.value("skin");
        if (!skinV.isObject()) {
            qWarning() << "skin is missing";
            return MinecraftProfile();
        }
        auto skinObj = skinV.toObject();
        auto idV = skinObj.value("id");
        auto urlV = skinObj.value("url");
        auto variantV = skinObj.value("variant");
        if (!idV.isString() || !urlV.isString() || !variantV.isString()) {
            qWarning() << "mandatory skin attributes are missing or of unexpected type";
            return MinecraftProfile();
        }
        out.skin.id = idV.toString();
        out.skin.url = urlV.toString();
        out.skin.variant = variantV.toString();

        // data for skin is optional
        auto dataV = skinObj.value("data");
        if (dataV.isString()) {
            // TODO: validate base64
            out.skin.data = QByteArray::fromBase64(dataV.toString().toLatin1());
        } else if (!dataV.isUndefined()) {
            qWarning() << "skin data is something unexpected";
            return MinecraftProfile();
        }
    }

    {
        auto capesV = tokenObject.value("capes");
        if (!capesV.isArray()) {
            qWarning() << "capes is not an array!";
            return MinecraftProfile();
        }
        auto capesArray = capesV.toArray();
        for (auto capeV : capesArray) {
            if (!capeV.isObject()) {
                qWarning() << "cape is not an object!";
                return MinecraftProfile();
            }
            auto capeObj = capeV.toObject();
            auto idV = capeObj.value("id");
            auto urlV = capeObj.value("url");
            auto aliasV = capeObj.value("alias");
            if (!idV.isString() || !urlV.isString() || !aliasV.isString()) {
                qWarning() << "mandatory skin attributes are missing or of unexpected type";
                return MinecraftProfile();
            }
            Cape cape;
            cape.id = idV.toString();
            cape.url = urlV.toString();
            cape.alias = aliasV.toString();

            // data for cape is optional.
            auto dataV = capeObj.value("data");
            if (dataV.isString()) {
                // TODO: validate base64
                cape.data = QByteArray::fromBase64(dataV.toString().toLatin1());
            } else if (!dataV.isUndefined()) {
                qWarning() << "cape data is something unexpected";
                return MinecraftProfile();
            }
            out.capes[cape.id] = cape;
        }
    }
    // current cape
    {
        auto capeV = tokenObject.value("cape");
        if (capeV.isString()) {
            auto currentCape = capeV.toString();
            if (out.capes.contains(currentCape)) {
                out.currentCape = currentCape;
            }
        }
    }
    out.validity = Validity::Assumed;
    return out;
}

void entitlementToJSONV3(QJsonObject& parent, MinecraftEntitlement p)
{
    if (p.validity == Validity::None) {
        return;
    }
    QJsonObject out;
    out["ownsMinecraft"] = QJsonValue(p.ownsMinecraft);
    out["canPlayMinecraft"] = QJsonValue(p.canPlayMinecraft);
    parent["entitlement"] = out;
}

bool entitlementFromJSONV3(const QJsonObject& parent, MinecraftEntitlement& out)
{
    auto entitlementObject = parent.value("entitlement").toObject();
    if (entitlementObject.isEmpty()) {
        return false;
    }
    {
        auto ownsMinecraftV = entitlementObject.value("ownsMinecraft");
        auto canPlayMinecraftV = entitlementObject.value("canPlayMinecraft");
        if (!ownsMinecraftV.isBool() || !canPlayMinecraftV.isBool()) {
            qWarning() << "mandatory attributes are missing or of unexpected type";
            return false;
        }
        out.canPlayMinecraft = canPlayMinecraftV.toBool(false);
        out.ownsMinecraft = ownsMinecraftV.toBool(false);
        out.validity = Validity::Assumed;
    }
    return true;
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
        msaToken = tokenFromJSONV3(data, "msa");
        userToken = tokenFromJSONV3(data, "utoken");
        xboxApiToken = tokenFromJSONV3(data, "xrp-main");
        mojangservicesToken = tokenFromJSONV3(data, "xrp-mc");
    }

    yggdrasilToken = tokenFromJSONV3(data, "ygg");
    // versions before 7.2 used "offline" as the offline token
    if (yggdrasilToken.token == "offline")
        yggdrasilToken.token = "0";

    minecraftProfile = profileFromJSONV3(data, "profile");
    if (!entitlementFromJSONV3(data, minecraftEntitlement)) {
        if (minecraftProfile.validity != Validity::None) {
            minecraftEntitlement.canPlayMinecraft = true;
            minecraftEntitlement.ownsMinecraft = true;
            minecraftEntitlement.validity = Validity::Assumed;
        }
    }

    validity_ = minecraftProfile.validity;
    return true;
}

QJsonObject AccountData::saveState() const
{
    QJsonObject output;
    if (type == AccountType::MSA) {
        output["type"] = "MSA";
        output["msa-client-id"] = msaClientID;
        tokenToJSONV3(output, msaToken, "msa");
        tokenToJSONV3(output, userToken, "utoken");
        tokenToJSONV3(output, xboxApiToken, "xrp-main");
        tokenToJSONV3(output, mojangservicesToken, "xrp-mc");
    } else if (type == AccountType::Offline) {
        output["type"] = "Offline";
    }

    tokenToJSONV3(output, yggdrasilToken, "ygg");
    profileToJSONV3(output, minecraftProfile, "profile");
    entitlementToJSONV3(output, minecraftEntitlement);
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
