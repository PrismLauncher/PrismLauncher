#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDesktopServices>
#include <QMetaEnum>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
#include <QUrlQuery>

#include "AuthContext.h"
#include "katabasis/Globals.h"
#include "AuthRequest.h"

#include "Parsers.h"

#include <Application.h>

using OAuth2 = Katabasis::DeviceFlow;
using Activity = Katabasis::Activity;

AuthContext::AuthContext(AccountData * data, QObject *parent) :
    AccountTask(data, parent)
{
}

void AuthContext::beginActivity(Activity activity) {
    if(isBusy()) {
        throw 0;
    }
    m_activity = activity;
    changeState(STATE_WORKING, "Initializing");
    emit activityChanged(m_activity);
}

void AuthContext::finishActivity() {
    if(!isBusy()) {
        throw 0;
    }
    m_activity = Katabasis::Activity::Idle;
    setStage(AuthStage::Complete);
    m_data->validity_ = m_data->minecraftProfile.validity;
    emit activityChanged(m_activity);
}

void AuthContext::initMSA() {
    if(m_oauth2) {
        return;
    }

    OAuth2::Options opts;
    opts.scope = "XboxLive.signin offline_access";
    opts.clientIdentifier = APPLICATION->msaClientId();
    opts.authorizationUrl = "https://login.microsoftonline.com/consumers/oauth2/v2.0/devicecode";
    opts.accessTokenUrl = "https://login.microsoftonline.com/consumers/oauth2/v2.0/token";

    // FIXME: OAuth2 is not aware of our fancy shared pointers
    m_oauth2 = new OAuth2(opts, m_data->msaToken, this, APPLICATION->network().get());

    connect(m_oauth2, &OAuth2::activityChanged, this, &AuthContext::onOAuthActivityChanged);
    connect(m_oauth2, &OAuth2::showVerificationUriAndCode, this, &AuthContext::showVerificationUriAndCode);
}

void AuthContext::initMojang() {
    if(m_yggdrasil) {
        return;
    }
    m_yggdrasil = new Yggdrasil(m_data, this);

    connect(m_yggdrasil, &Task::failed, this, &AuthContext::onMojangFailed);
    connect(m_yggdrasil, &Task::succeeded, this, &AuthContext::onMojangSucceeded);
}

void AuthContext::onMojangSucceeded() {
    doMinecraftProfile();
}


void AuthContext::onMojangFailed() {
    finishActivity();
    m_error = m_yggdrasil->m_error;
    m_aborted = m_yggdrasil->m_aborted;
    changeState(m_yggdrasil->accountState(), tr("Mojang user authentication failed."));
}

void AuthContext::onOAuthActivityChanged(Katabasis::Activity activity) {
    switch(activity) {
        case Katabasis::Activity::Idle:
        case Katabasis::Activity::LoggingIn:
        case Katabasis::Activity::Refreshing:
        case Katabasis::Activity::LoggingOut: {
            // We asked it to do something, it's doing it. Nothing to act upon.
            return;
        }
        case Katabasis::Activity::Succeeded: {
            // Succeeded or did not invalidate tokens
            emit hideVerificationUriAndCode();
            if (!m_oauth2->linked()) {
                finishActivity();
                changeState(STATE_FAILED_HARD, tr("Microsoft user authentication ended with an impossible state (succeeded, but not succeeded at the same time)."));
                return;
            }
            QVariantMap extraTokens = m_oauth2->extraTokens();
#ifndef NDEBUG
            if (!extraTokens.isEmpty()) {
                qDebug() << "Extra tokens in response:";
                foreach (QString key, extraTokens.keys()) {
                    qDebug() << "\t" << key << ":" << extraTokens.value(key);
                }
            }
#endif
            doUserAuth();
            return;
        }
        case Katabasis::Activity::FailedSoft: {
            emit hideVerificationUriAndCode();
            finishActivity();
            changeState(STATE_FAILED_SOFT, tr("Microsoft user authentication failed with a soft error."));
            return;
        }
        case Katabasis::Activity::FailedGone:
        case Katabasis::Activity::FailedHard: {
            emit hideVerificationUriAndCode();
            finishActivity();
            changeState(STATE_FAILED_HARD, tr("Microsoft user authentication failed."));
            return;
        }
        default: {
            emit hideVerificationUriAndCode();
            finishActivity();
            changeState(STATE_FAILED_HARD, tr("Microsoft user authentication completed with an unrecognized result."));
            return;
        }

    }
}

void AuthContext::doUserAuth() {
    setStage(AuthStage::UserAuth);
    changeState(STATE_WORKING, tr("Starting user authentication"));

    QString xbox_auth_template = R"XXX(
{
    "Properties": {
        "AuthMethod": "RPS",
        "SiteName": "user.auth.xboxlive.com",
        "RpsTicket": "d=%1"
    },
    "RelyingParty": "http://auth.xboxlive.com",
    "TokenType": "JWT"
}
)XXX";
    auto xbox_auth_data = xbox_auth_template.arg(m_data->msaToken.token);

    QNetworkRequest request = QNetworkRequest(QUrl("https://user.auth.xboxlive.com/user/authenticate"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    auto *requestor = new AuthRequest(this);
    connect(requestor, &AuthRequest::finished, this, &AuthContext::onUserAuthDone);
    requestor->post(request, xbox_auth_data.toUtf8());
    qDebug() << "First layer of XBox auth ... commencing.";
}

void AuthContext::onUserAuthDone(
    QNetworkReply::NetworkError error,
    QByteArray replyData,
    QList<QNetworkReply::RawHeaderPair> headers
) {
    if (error != QNetworkReply::NoError) {
        qWarning() << "Reply error:" << error;
        finishActivity();
        changeState(STATE_FAILED_HARD, tr("XBox user authentication failed."));
        return;
    }

    Katabasis::Token temp;
    if(!Parsers::parseXTokenResponse(replyData, temp, "UToken")) {
        qWarning() << "Could not parse user authentication response...";
        finishActivity();
        changeState(STATE_FAILED_HARD, tr("XBox user authentication response could not be understood."));
        return;
    }
    m_data->userToken = temp;

    setStage(AuthStage::XboxAuth);
    changeState(STATE_WORKING, tr("Starting XBox authentication"));

    doSTSAuthMinecraft();
    doSTSAuthGeneric();
}
/*
        url = "https://xsts.auth.xboxlive.com/xsts/authorize"
        headers = {"x-xbl-contract-version": "1"}
        data = {
            "RelyingParty": relying_party,
            "TokenType": "JWT",
            "Properties": {
                "UserTokens": [self.user_token.token],
                "SandboxId": "RETAIL",
            },
        }
*/
void AuthContext::doSTSAuthMinecraft() {
    QString xbox_auth_template = R"XXX(
{
    "Properties": {
        "SandboxId": "RETAIL",
        "UserTokens": [
            "%1"
        ]
    },
    "RelyingParty": "rp://api.minecraftservices.com/",
    "TokenType": "JWT"
}
)XXX";
    auto xbox_auth_data = xbox_auth_template.arg(m_data->userToken.token);

    QNetworkRequest request = QNetworkRequest(QUrl("https://xsts.auth.xboxlive.com/xsts/authorize"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    AuthRequest *requestor = new AuthRequest(this);
    connect(requestor, &AuthRequest::finished, this, &AuthContext::onSTSAuthMinecraftDone);
    requestor->post(request, xbox_auth_data.toUtf8());
    qDebug() << "Getting Minecraft services STS token...";
}

void AuthContext::processSTSError(QNetworkReply::NetworkError error, QByteArray data, QList<QNetworkReply::RawHeaderPair> headers) {
    if(error == QNetworkReply::AuthenticationRequiredError) {
        QJsonParseError jsonError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
        if(jsonError.error) {
            qWarning() << "Cannot parse error XSTS response as JSON: " << jsonError.errorString();
            return;
        }

        int64_t errorCode = -1;
        auto obj = doc.object();
        if(!Parsers::getNumber(obj.value("XErr"), errorCode)) {
            qWarning() << "XErr is not a number";
            return;
        }
        stsErrors.insert(errorCode);
        stsFailed = true;
    }
}


void AuthContext::onSTSAuthMinecraftDone(
    QNetworkReply::NetworkError error,
    QByteArray replyData,
    QList<QNetworkReply::RawHeaderPair> headers
) {
#ifndef NDEBUG
    qDebug() << replyData;
#endif
    if (error != QNetworkReply::NoError) {
        qWarning() << "Reply error:" << error;
        processSTSError(error, replyData, headers);
        failResult(m_mcAuthSucceeded);
        return;
    }

    Katabasis::Token temp;
    if(!Parsers::parseXTokenResponse(replyData, temp, "STSAuthMinecraft")) {
        qWarning() << "Could not parse authorization response for access to mojang services...";
        failResult(m_mcAuthSucceeded);
        return;
    }

    if(temp.extra["uhs"] != m_data->userToken.extra["uhs"]) {
        qWarning() << "Server has changed user hash in the reply... something is wrong. ABORTING";
        failResult(m_mcAuthSucceeded);
        return;
    }
    m_data->mojangservicesToken = temp;

    doMinecraftAuth();
}

void AuthContext::doMinecraftAuth() {
    auto requestURL = "https://api.minecraftservices.com/launcher/login";
    auto uhs = m_data->mojangservicesToken.extra["uhs"].toString();
    auto xToken = m_data->mojangservicesToken.token;

    QString mc_auth_template = R"XXX(
{
    "xtoken": "XBL3.0 x=%1;%2",
    "platform": "PC_LAUNCHER"
}
)XXX";
    auto requestBody = mc_auth_template.arg(uhs, xToken);

    QNetworkRequest request = QNetworkRequest(QUrl(requestURL));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    AuthRequest *requestor = new AuthRequest(this);
    connect(requestor, &AuthRequest::finished, this, &AuthContext::onMinecraftAuthDone);
    requestor->post(request, requestBody.toUtf8());
    qDebug() << "Getting Minecraft access token...";
}

void AuthContext::onMinecraftAuthDone(
    QNetworkReply::NetworkError error,
    QByteArray replyData,
    QList<QNetworkReply::RawHeaderPair> headers
) {
    qDebug() << replyData;
    if (error != QNetworkReply::NoError) {
        qWarning() << "Reply error:" << error;
#ifndef NDEBUG
        qDebug() << replyData;
#endif
        failResult(m_mcAuthSucceeded);
        return;
    }

    if(!Parsers::parseMojangResponse(replyData, m_data->yggdrasilToken)) {
        qWarning() << "Could not parse login_with_xbox response...";
#ifndef NDEBUG
        qDebug() << replyData;
#endif
        failResult(m_mcAuthSucceeded);
        return;
    }

    succeedResult(m_mcAuthSucceeded);
}

void AuthContext::doSTSAuthGeneric() {
    QString xbox_auth_template = R"XXX(
{
    "Properties": {
        "SandboxId": "RETAIL",
        "UserTokens": [
            "%1"
        ]
    },
    "RelyingParty": "http://xboxlive.com",
    "TokenType": "JWT"
}
)XXX";
    auto xbox_auth_data = xbox_auth_template.arg(m_data->userToken.token);

    QNetworkRequest request = QNetworkRequest(QUrl("https://xsts.auth.xboxlive.com/xsts/authorize"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    AuthRequest *requestor = new AuthRequest(this);
    connect(requestor, &AuthRequest::finished, this, &AuthContext::onSTSAuthGenericDone);
    requestor->post(request, xbox_auth_data.toUtf8());
    qDebug() << "Getting generic STS token...";
}

void AuthContext::onSTSAuthGenericDone(
    QNetworkReply::NetworkError error,
    QByteArray replyData,
    QList<QNetworkReply::RawHeaderPair> headers
) {
#ifndef NDEBUG
    qDebug() << replyData;
#endif
    if (error != QNetworkReply::NoError) {
        qWarning() << "Reply error:" << error;
        processSTSError(error, replyData, headers);
        failResult(m_xboxProfileSucceeded);
        return;
    }

    Katabasis::Token temp;
    if(!Parsers::parseXTokenResponse(replyData, temp, "STSAuthGeneric")) {
        qWarning() << "Could not parse authorization response for access to xbox API...";
        failResult(m_xboxProfileSucceeded);
        return;
    }

    if(temp.extra["uhs"] != m_data->userToken.extra["uhs"]) {
        qWarning() << "Server has changed user hash in the reply... something is wrong. ABORTING";
        failResult(m_xboxProfileSucceeded);
        return;
    }
    m_data->xboxApiToken = temp;

    doXBoxProfile();
}

void AuthContext::doXBoxProfile() {
    auto url = QUrl("https://profile.xboxlive.com/users/me/profile/settings");
    QUrlQuery q;
    q.addQueryItem(
        "settings",
        "GameDisplayName,AppDisplayName,AppDisplayPicRaw,GameDisplayPicRaw,"
        "PublicGamerpic,ShowUserAsAvatar,Gamerscore,Gamertag,ModernGamertag,ModernGamertagSuffix,"
        "UniqueModernGamertag,AccountTier,TenureLevel,XboxOneRep,"
        "PreferredColor,Location,Bio,Watermarks,"
        "RealName,RealNameOverride,IsQuarantined"
    );
    url.setQuery(q);

    QNetworkRequest request = QNetworkRequest(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("x-xbl-contract-version", "3");
    request.setRawHeader("Authorization", QString("XBL3.0 x=%1;%2").arg(m_data->userToken.extra["uhs"].toString(), m_data->xboxApiToken.token).toUtf8());
    AuthRequest *requestor = new AuthRequest(this);
    connect(requestor, &AuthRequest::finished, this, &AuthContext::onXBoxProfileDone);
    requestor->get(request);
    qDebug() << "Getting Xbox profile...";
}

void AuthContext::onXBoxProfileDone(
    QNetworkReply::NetworkError error,
    QByteArray replyData,
    QList<QNetworkReply::RawHeaderPair> headers
) {
    if (error != QNetworkReply::NoError) {
        qWarning() << "Reply error:" << error;
#ifndef NDEBUG
        qDebug() << replyData;
#endif
        failResult(m_xboxProfileSucceeded);
        return;
    }

#ifndef NDEBUG
    qDebug() << "XBox profile: " << replyData;
#endif

    succeedResult(m_xboxProfileSucceeded);
}

void AuthContext::succeedResult(bool& flag) {
    m_requestsDone ++;
    flag = true;
    checkResult();
}

void AuthContext::failResult(bool& flag) {
    m_requestsDone ++;
    flag = false;
    checkResult();
}

void AuthContext::checkResult() {
    qDebug() << "AuthContext::checkResult called";
    if(m_requestsDone != 2) {
        qDebug() << "Number of ready results:" << m_requestsDone;
        return;
    }
    if(m_mcAuthSucceeded && m_xboxProfileSucceeded) {
        doEntitlements();
    }
    else {
        finishActivity();
        if(stsFailed) {
            if(stsErrors.contains(2148916233)) {
                changeState(
                    STATE_FAILED_HARD,
                    tr("This Microsoft account does not have an XBox Live profile. Buy the game on %1 first.")
                        .arg("<a href=\"https://www.minecraft.net/en-us/store/minecraft-java-edition\">minecraft.net</a>")
                );
            }
            else if (stsErrors.contains(2148916235)){
                // NOTE: this is the Grulovia error
                changeState(
                    STATE_FAILED_HARD,
                    tr("XBox Live is not available in your country. You've been blocked.")
                );
            }
            else if (stsErrors.contains(2148916238)){
                changeState(
                    STATE_FAILED_HARD,
                    tr("This Microsoft account is underaged and is not linked to a family.\n\nPlease set up your account according to %1.")
                        .arg("<a href=\"https://help.minecraft.net/hc/en-us/articles/4403181904525\">help.minecraft.net</a>")
                );
            }
            else {
                QStringList errorList;
                for(auto & error: stsErrors) {
                    errorList.append(QString::number(error));
                }
                changeState(
                    STATE_FAILED_HARD,
                    tr("XSTS authentication ended with unrecognized error(s):\n\n%1").arg(errorList.join("\n"))
                );
            }
        }
        else {
            changeState(STATE_FAILED_HARD, tr("XBox and/or Mojang authentication steps did not succeed"));
        }
    }
}

void AuthContext::doEntitlements() {
    auto uuid = QUuid::createUuid();
    entitlementsRequestId = uuid.toString().remove('{').remove('}');
    auto url = "https://api.minecraftservices.com/entitlements/license?requestId=" + entitlementsRequestId;
    QNetworkRequest request = QNetworkRequest(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_data->yggdrasilToken.token).toUtf8());
    AuthRequest *requestor = new AuthRequest(this);
    connect(requestor, &AuthRequest::finished, this, &AuthContext::onEntitlementsDone);
    requestor->get(request);
    qDebug() << "Getting Xbox profile...";
}


void AuthContext::onEntitlementsDone(
    QNetworkReply::NetworkError error,
    QByteArray data,
    QList<QNetworkReply::RawHeaderPair> headers
) {
#ifndef NDEBUG
    qDebug() << data;
#endif
    // TODO: check presence of same entitlementsRequestId?
    // TODO: validate JWTs?
    Parsers::parseMinecraftEntitlements(data, m_data->minecraftEntitlement);
    doMinecraftProfile();
}

void AuthContext::doMinecraftProfile() {
    setStage(AuthStage::MinecraftProfile);
    changeState(STATE_WORKING, tr("Starting minecraft profile acquisition"));

    auto url = QUrl("https://api.minecraftservices.com/minecraft/profile");
    QNetworkRequest request = QNetworkRequest(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    // request.setRawHeader("Accept", "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_data->yggdrasilToken.token).toUtf8());

    AuthRequest *requestor = new AuthRequest(this);
    connect(requestor, &AuthRequest::finished, this, &AuthContext::onMinecraftProfileDone);
    requestor->get(request);
}

void AuthContext::onMinecraftProfileDone(
    QNetworkReply::NetworkError error,
    QByteArray data,
    QList<QNetworkReply::RawHeaderPair> headers
) {
#ifndef NDEBUG
    qDebug() << data;
#endif
    if (error == QNetworkReply::ContentNotFoundError) {
        // NOTE: Succeed even if we do not have a profile. This is a valid account state.
        if(m_data->type == AccountType::Mojang) {
            m_data->minecraftEntitlement.canPlayMinecraft = false;
            m_data->minecraftEntitlement.ownsMinecraft = false;
        }
        m_data->minecraftProfile = MinecraftProfile();
        succeed();
        return;
    }
    if (error != QNetworkReply::NoError) {
        finishActivity();
        changeState(STATE_FAILED_HARD, tr("Minecraft Java profile acquisition failed."));
        return;
    }
    if(!Parsers::parseMinecraftProfile(data, m_data->minecraftProfile)) {
        m_data->minecraftProfile = MinecraftProfile();
        finishActivity();
        changeState(STATE_FAILED_HARD, tr("Minecraft Java profile response could not be parsed"));
        return;
    }

    if(m_data->type == AccountType::Mojang) {
        auto validProfile = m_data->minecraftProfile.validity == Katabasis::Validity::Certain;
        m_data->minecraftEntitlement.canPlayMinecraft = validProfile;
        m_data->minecraftEntitlement.ownsMinecraft = validProfile;
        doMigrationEligibilityCheck();
    }
    else {
        doGetSkin();
    }
}

void AuthContext::doMigrationEligibilityCheck() {
    setStage(AuthStage::MigrationEligibility);
    changeState(STATE_WORKING, tr("Starting check for migration eligibility"));

    auto url = QUrl("https://api.minecraftservices.com/rollout/v1/msamigration");
    QNetworkRequest request = QNetworkRequest(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_data->yggdrasilToken.token).toUtf8());

    AuthRequest *requestor = new AuthRequest(this);
    connect(requestor, &AuthRequest::finished, this, &AuthContext::onMigrationEligibilityCheckDone);
    requestor->get(request);
}

void AuthContext::onMigrationEligibilityCheckDone(
    QNetworkReply::NetworkError error,
    QByteArray data,
    QList<QNetworkReply::RawHeaderPair> headers
) {
    if (error == QNetworkReply::NoError) {
        Parsers::parseRolloutResponse(data, m_data->canMigrateToMSA);
    }
    doGetSkin();
}

void AuthContext::doGetSkin() {
    setStage(AuthStage::Skin);
    changeState(STATE_WORKING, tr("Fetching player skin"));

    auto url = QUrl(m_data->minecraftProfile.skin.url);
    QNetworkRequest request = QNetworkRequest(url);
    AuthRequest *requestor = new AuthRequest(this);
    connect(requestor, &AuthRequest::finished, this, &AuthContext::onSkinDone);
    requestor->get(request);
}

void AuthContext::onSkinDone(
    QNetworkReply::NetworkError error,
    QByteArray data,
    QList<QNetworkReply::RawHeaderPair>
) {
    if (error == QNetworkReply::NoError) {
        m_data->minecraftProfile.skin.data = data;
    }
    succeed();

}

void AuthContext::succeed() {
    m_data->validity_ = Katabasis::Validity::Certain;
    finishActivity();
    changeState(STATE_SUCCEEDED, tr("Finished all authentication steps"));
}

void AuthContext::setStage(AuthContext::AuthStage stage) {
    m_stage = stage;
    emit progress((int)m_stage, (int)AuthStage::Complete);
}


QString AuthContext::getStateMessage() const {
    switch (m_accountState)
    {
        case STATE_WORKING:
            switch(m_stage) {
                case AuthStage::Initial: {
                    QString loginMessage = tr("Logging in as %1 user");
                    if(m_data->type == AccountType::MSA) {
                        return loginMessage.arg("Microsoft");
                    }
                    else {
                        return loginMessage.arg("Mojang");
                    }
                }
                case AuthStage::UserAuth:
                    return tr("Logging in as XBox user");
                case AuthStage::XboxAuth:
                    return tr("Logging in with XBox and Mojang services");
                case AuthStage::MinecraftProfile:
                    return tr("Getting Minecraft profile");
                case AuthStage::MigrationEligibility:
                    return tr("Checking for migration eligibility");
                case AuthStage::Skin:
                    return tr("Getting Minecraft skin");
                case AuthStage::Complete:
                    return tr("Finished");
                default:
                    break;
            }
        default:
            return AccountTask::getStateMessage();
    }
}
