#include "AccountData.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QUuid>

namespace {
void tokenToJSONV3(QJsonObject &parent, Katabasis::Token t, const char * tokenName) {
    if(!t.persistent) {
        return;
    }
    QJsonObject out;
    if(t.issueInstant.isValid()) {
        out["iat"] = QJsonValue(t.issueInstant.toMSecsSinceEpoch() / 1000);
    }

    if(t.notAfter.isValid()) {
        out["exp"] = QJsonValue(t.notAfter.toMSecsSinceEpoch() / 1000);
    }

    bool save = false;
    if(!t.token.isEmpty()) {
        out["token"] = QJsonValue(t.token);
        save = true;
    }
    if(!t.refresh_token.isEmpty()) {
        out["refresh_token"] = QJsonValue(t.refresh_token);
        save = true;
    }
    if(t.extra.size()) {
        out["extra"] = QJsonObject::fromVariantMap(t.extra);
        save = true;
    }
    if(save) {
        parent[tokenName] = out;
    }
}

Katabasis::Token tokenFromJSONV3(const QJsonObject &parent, const char * tokenName) {
    Katabasis::Token out;
    auto tokenObject = parent.value(tokenName).toObject();
    if(tokenObject.isEmpty()) {
        return out;
    }
    auto issueInstant = tokenObject.value("iat");
    if(issueInstant.isDouble()) {
        out.issueInstant = QDateTime::fromMSecsSinceEpoch(((int64_t) issueInstant.toDouble()) * 1000);
    }

    auto notAfter = tokenObject.value("exp");
    if(notAfter.isDouble()) {
        out.notAfter = QDateTime::fromMSecsSinceEpoch(((int64_t) notAfter.toDouble()) * 1000);
    }

    auto token = tokenObject.value("token");
    if(token.isString()) {
        out.token = token.toString();
        out.validity = Katabasis::Validity::Assumed;
    }

    auto refresh_token = tokenObject.value("refresh_token");
    if(refresh_token.isString()) {
        out.refresh_token = refresh_token.toString();
    }

    auto extra = tokenObject.value("extra");
    if(extra.isObject()) {
        out.extra = extra.toObject().toVariantMap();
    }
    return out;
}

void profileToJSONV3(QJsonObject &parent, MinecraftProfile p, const char * tokenName) {
    if(p.id.isEmpty()) {
        return;
    }
    QJsonObject out;
    out["id"] = QJsonValue(p.id);
    out["name"] = QJsonValue(p.name);
    if(!p.currentCape.isEmpty()) {
        out["cape"] = p.currentCape;
    }

    {
        QJsonObject skinObj;
        skinObj["id"] = p.skin.id;
        skinObj["url"] = p.skin.url;
        skinObj["variant"] = p.skin.variant;
        if(p.skin.data.size()) {
            skinObj["data"] = QString::fromLatin1(p.skin.data.toBase64());
        }
        out["skin"] = skinObj;
    }

    QJsonArray capesArray;
    for(auto & cape: p.capes) {
        QJsonObject capeObj;
        capeObj["id"] = cape.id;
        capeObj["url"] = cape.url;
        capeObj["alias"] = cape.alias;
        if(cape.data.size()) {
            capeObj["data"] = QString::fromLatin1(cape.data.toBase64());
        }
        capesArray.push_back(capeObj);
    }
    out["capes"] = capesArray;
    parent[tokenName] = out;
}

MinecraftProfile profileFromJSONV3(const QJsonObject &parent, const char * tokenName) {
    MinecraftProfile out;
    auto tokenObject = parent.value(tokenName).toObject();
    if(tokenObject.isEmpty()) {
        return out;
    }
    {
        auto idV = tokenObject.value("id");
        auto nameV = tokenObject.value("name");
        if(!idV.isString() || !nameV.isString()) {
            qWarning() << "mandatory profile attributes are missing or of unexpected type";
            return MinecraftProfile();
        }
        out.name = nameV.toString();
        out.id = idV.toString();
    }

    {
        auto skinV = tokenObject.value("skin");
        if(!skinV.isObject()) {
            qWarning() << "skin is missing";
            return MinecraftProfile();
        }
        auto skinObj = skinV.toObject();
        auto idV = skinObj.value("id");
        auto urlV = skinObj.value("url");
        auto variantV = skinObj.value("variant");
        if(!idV.isString() || !urlV.isString() || !variantV.isString()) {
            qWarning() << "mandatory skin attributes are missing or of unexpected type";
            return MinecraftProfile();
        }
        out.skin.id = idV.toString();
        out.skin.url = urlV.toString();
        out.skin.variant = variantV.toString();

        // data for skin is optional
        auto dataV = skinObj.value("data");
        if(dataV.isString()) {
            // TODO: validate base64
            out.skin.data = QByteArray::fromBase64(dataV.toString().toLatin1());
        }
        else if (!dataV.isUndefined()) {
            qWarning() << "skin data is something unexpected";
            return MinecraftProfile();
        }
    }

    {
        auto capesV = tokenObject.value("capes");
        if(!capesV.isArray()) {
            qWarning() << "capes is not an array!";
            return MinecraftProfile();
        }
        auto capesArray = capesV.toArray();
        for(auto capeV: capesArray) {
            if(!capeV.isObject()) {
                qWarning() << "cape is not an object!";
                return MinecraftProfile();
            }
            auto capeObj = capeV.toObject();
            auto idV = capeObj.value("id");
            auto urlV = capeObj.value("url");
            auto aliasV = capeObj.value("alias");
            if(!idV.isString() || !urlV.isString() || !aliasV.isString()) {
                qWarning() << "mandatory skin attributes are missing or of unexpected type";
                return MinecraftProfile();
            }
            Cape cape;
            cape.id = idV.toString();
            cape.url = urlV.toString();
            cape.alias = aliasV.toString();

            // data for cape is optional.
            auto dataV = capeObj.value("data");
            if(dataV.isString()) {
                // TODO: validate base64
                cape.data = QByteArray::fromBase64(dataV.toString().toLatin1());
            }
            else if (!dataV.isUndefined()) {
                qWarning() << "cape data is something unexpected";
                return MinecraftProfile();
            }
            out.capes[cape.id] = cape;
        }
    }
    // current cape
    {
        auto capeV = tokenObject.value("cape");
        if(capeV.isString()) {
            auto currentCape = capeV.toString();
            if(out.capes.contains(currentCape)) {
                out.currentCape = currentCape;
            }
        }
    }
    out.validity = Katabasis::Validity::Assumed;
    return out;
}

void entitlementToJSONV3(QJsonObject &parent, MinecraftEntitlement p) {
    if(p.validity == Katabasis::Validity::None) {
        return;
    }
    QJsonObject out;
    out["ownsMinecraft"] = QJsonValue(p.ownsMinecraft);
    out["canPlayMinecraft"] = QJsonValue(p.canPlayMinecraft);
    parent["entitlement"] = out;
}

bool entitlementFromJSONV3(const QJsonObject &parent, MinecraftEntitlement & out) {
    auto entitlementObject = parent.value("entitlement").toObject();
    if(entitlementObject.isEmpty()) {
        return false;
    }
    {
        auto ownsMinecraftV = entitlementObject.value("ownsMinecraft");
        auto canPlayMinecraftV = entitlementObject.value("canPlayMinecraft");
        if(!ownsMinecraftV.isBool() || !canPlayMinecraftV.isBool()) {
            qWarning() << "mandatory attributes are missing or of unexpected type";
            return false;
        }
        out.canPlayMinecraft = canPlayMinecraftV.toBool(false);
        out.ownsMinecraft = ownsMinecraftV.toBool(false);
        out.validity = Katabasis::Validity::Assumed;
    }
    return true;
}

}

bool AccountData::resumeStateFromV2(QJsonObject data) {
    // The JSON object must at least have a username for it to be valid.
    if (!data.value("username").isString())
    {
        qCritical() << "Can't load Mojang account info from JSON object. Username field is missing or of the wrong type.";
        return false;
    }

    QString userName = data.value("username").toString("");
    QString clientToken = data.value("clientToken").toString("");
    QString accessToken = data.value("accessToken").toString("");

    QJsonArray profileArray = data.value("profiles").toArray();
    if (profileArray.size() < 1)
    {
        qCritical() << "Can't load Mojang account with username \"" << userName << "\". No profiles found.";
        return false;
    }

    struct AccountProfile
    {
        QString id;
        QString name;
        bool legacy;
    };

    QList<AccountProfile> profiles;
    int currentProfileIndex = 0;
    int index = -1;
    QString currentProfile = data.value("activeProfile").toString("");
    for (QJsonValue profileVal : profileArray)
    {
        index++;
        QJsonObject profileObject = profileVal.toObject();
        QString id = profileObject.value("id").toString("");
        QString name = profileObject.value("name").toString("");
        bool legacy = profileObject.value("legacy").toBool(false);
        if (id.isEmpty() || name.isEmpty())
        {
            qWarning() << "Unable to load a profile" << name << "because it was missing an ID or a name.";
            continue;
        }
        if(id == currentProfile) {
            currentProfileIndex = index;
        }
        profiles.append({id, name, legacy});
    }
    auto & profile = profiles[currentProfileIndex];

    type = AccountType::Mojang;
    legacy = profile.legacy;

    minecraftProfile.id = profile.id;
    minecraftProfile.name = profile.name;
    minecraftProfile.validity = Katabasis::Validity::Assumed;

    yggdrasilToken.token = accessToken;
    yggdrasilToken.extra["clientToken"] = clientToken;
    yggdrasilToken.extra["userName"] = userName;
    yggdrasilToken.validity = Katabasis::Validity::Assumed;

    validity_ = minecraftProfile.validity;
    return true;
}

bool AccountData::resumeStateFromV3(QJsonObject data) {
    auto typeV = data.value("type");
    if(!typeV.isString()) {
        qWarning() << "Failed to parse account data: type is missing.";
        return false;
    }
    auto typeS = typeV.toString();
    if(typeS == "MSA") {
        type = AccountType::MSA;
    } else if (typeS == "Mojang") {
        type = AccountType::Mojang;
    } else if (typeS == "Offline") {
        type = AccountType::Offline;
    } else {
        qWarning() << "Failed to parse account data: type is not recognized.";
        return false;
    }

    if(type == AccountType::Mojang) {
        legacy = data.value("legacy").toBool(false);
        canMigrateToMSA = data.value("canMigrateToMSA").toBool(false);
    }

    if(type == AccountType::MSA) {
        msaToken = tokenFromJSONV3(data, "msa");
        userToken = tokenFromJSONV3(data, "utoken");
        xboxApiToken = tokenFromJSONV3(data, "xrp-main");
        mojangservicesToken = tokenFromJSONV3(data, "xrp-mc");
    }

    yggdrasilToken = tokenFromJSONV3(data, "ygg");
    minecraftProfile = profileFromJSONV3(data, "profile");
    if(!entitlementFromJSONV3(data, minecraftEntitlement)) {
        if(minecraftProfile.validity != Katabasis::Validity::None) {
            minecraftEntitlement.canPlayMinecraft = true;
            minecraftEntitlement.ownsMinecraft = true;
            minecraftEntitlement.validity = Katabasis::Validity::Assumed;
        }
    }

    validity_ = minecraftProfile.validity;
    return true;
}

QJsonObject AccountData::saveState() const {
    QJsonObject output;
    if(type == AccountType::Mojang) {
        output["type"] = "Mojang";
        if(legacy) {
            output["legacy"] = true;
        }
        if(canMigrateToMSA) {
            output["canMigrateToMSA"] = true;
        }
    }
    else if (type == AccountType::MSA) {
        output["type"] = "MSA";
        tokenToJSONV3(output, msaToken, "msa");
        tokenToJSONV3(output, userToken, "utoken");
        tokenToJSONV3(output, xboxApiToken, "xrp-main");
        tokenToJSONV3(output, mojangservicesToken, "xrp-mc");
    }
    else if (type == AccountType::Offline) {
        output["type"] = "Offline";
    }

    tokenToJSONV3(output, yggdrasilToken, "ygg");
    profileToJSONV3(output, minecraftProfile, "profile");
    entitlementToJSONV3(output, minecraftEntitlement);
    return output;
}

QString AccountData::userName() const {
    if(type == AccountType::MSA) {
        return QString();
    }
    return yggdrasilToken.extra["userName"].toString();
}

QString AccountData::accessToken() const {
    return yggdrasilToken.token;
}

QString AccountData::clientToken() const {
    if(type != AccountType::Mojang) {
        return QString();
    }
    return yggdrasilToken.extra["clientToken"].toString();
}

void AccountData::setClientToken(QString clientToken) {
    if(type != AccountType::Mojang) {
        return;
    }
    yggdrasilToken.extra["clientToken"] = clientToken;
}

void AccountData::generateClientTokenIfMissing() {
    if(yggdrasilToken.extra.contains("clientToken")) {
        return;
    }
    invalidateClientToken();
}

void AccountData::invalidateClientToken() {
    if(type != AccountType::Mojang) {
        return;
    }
    yggdrasilToken.extra["clientToken"] = QUuid::createUuid().toString().remove(QRegExp("[{-}]"));
}

QString AccountData::profileId() const {
    return minecraftProfile.id;
}

QString AccountData::profileName() const {
    if(minecraftProfile.name.size() == 0) {
        return QObject::tr("No profile (%1)").arg(accountDisplayString());
    }
    else {
        return minecraftProfile.name;
    }
}

QString AccountData::accountDisplayString() const {
    switch(type) {
        case AccountType::Mojang: {
            return userName();
        }
        case AccountType::Offline: {
            return userName();
        }
        case AccountType::MSA: {
            if(xboxApiToken.extra.contains("gtg")) {
                return xboxApiToken.extra["gtg"].toString();
            }
            return "Xbox profile missing";
        }
        default: {
            return "Invalid Account";
        }
    }
}

QString AccountData::lastError() const {
    return errorString;
}
