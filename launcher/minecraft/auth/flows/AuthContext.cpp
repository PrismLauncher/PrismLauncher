#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDesktopServices>
#include <QMetaEnum>
#include <QDebug>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QUrlQuery>

#include <QPixmap>
#include <QPainter>

#include "AuthContext.h"
#include "katabasis/Globals.h"
#include "katabasis/Requestor.h"
#include "BuildConfig.h"

using OAuth2 = Katabasis::OAuth2;
using Requestor = Katabasis::Requestor;
using Activity = Katabasis::Activity;

AuthContext::AuthContext(AccountData * data, QObject *parent) :
    AccountTask(data, parent)
{
    mgr = new QNetworkAccessManager(this);
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
    m_stage = MSAStage::Idle;
    m_data->validity_ = m_data->minecraftProfile.validity;
    emit activityChanged(m_activity);
}

void AuthContext::initMSA() {
    if(m_oauth2) {
        return;
    }
    Katabasis::OAuth2::Options opts;
    opts.scope = "XboxLive.signin offline_access";
    opts.clientIdentifier = BuildConfig.MSA_CLIENT_ID;
    opts.authorizationUrl = "https://login.live.com/oauth20_authorize.srf";
    opts.accessTokenUrl = "https://login.live.com/oauth20_token.srf";
    opts.listenerPorts = {28562, 28563, 28564, 28565, 28566};

    m_oauth2 = new OAuth2(opts, m_data->msaToken, this, mgr);

    connect(m_oauth2, &OAuth2::linkingFailed, this, &AuthContext::onOAuthLinkingFailed);
    connect(m_oauth2, &OAuth2::linkingSucceeded, this, &AuthContext::onOAuthLinkingSucceeded);
    connect(m_oauth2, &OAuth2::openBrowser, this, &AuthContext::onOpenBrowser);
    connect(m_oauth2, &OAuth2::closeBrowser, this, &AuthContext::onCloseBrowser);
    connect(m_oauth2, &OAuth2::activityChanged, this, &AuthContext::onOAuthActivityChanged);
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

/*
bool AuthContext::signOut() {
    if(isBusy()) {
        return false;
    }

    start();

    beginActivity(Activity::LoggingOut);
    m_oauth2->unlink();
    m_account = AccountData();
    finishActivity();
    return true;
}
*/

void AuthContext::onOpenBrowser(const QUrl &url) {
    QDesktopServices::openUrl(url);
}

void AuthContext::onCloseBrowser() {

}

void AuthContext::onOAuthLinkingFailed() {
    finishActivity();
    changeState(STATE_FAILED_HARD, tr("Microsoft user authentication failed."));
}

void AuthContext::onOAuthLinkingSucceeded() {
    auto *o2t = qobject_cast<OAuth2 *>(sender());
    if (!o2t->linked()) {
        finishActivity();
        changeState(STATE_FAILED_HARD, tr("Microsoft user authentication ended with an impossible state (succeeded, but not succeeded at the same time)."));
        return;
    }
    QVariantMap extraTokens = o2t->extraTokens();
#ifndef NDEBUG
    if (!extraTokens.isEmpty()) {
        qDebug() << "Extra tokens in response:";
        foreach (QString key, extraTokens.keys()) {
            qDebug() << "\t" << key << ":" << extraTokens.value(key);
        }
    }
#endif
    doUserAuth();
}

void AuthContext::onOAuthActivityChanged(Katabasis::Activity activity) {
    // respond to activity change here
}

void AuthContext::doUserAuth() {
    m_stage = MSAStage::UserAuth;
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
    auto *requestor = new Katabasis::Requestor(mgr, m_oauth2, this);
    requestor->setAddAccessTokenInQuery(false);

    connect(requestor, &Requestor::finished, this, &AuthContext::onUserAuthDone);
    requestor->post(request, xbox_auth_data.toUtf8());
    qDebug() << "First layer of XBox auth ... commencing.";
}

namespace {
bool getDateTime(QJsonValue value, QDateTime & out) {
    if(!value.isString()) {
        return false;
    }
    out = QDateTime::fromString(value.toString(), Qt::ISODate);
    return out.isValid();
}

bool getString(QJsonValue value, QString & out) {
    if(!value.isString()) {
        return false;
    }
    out = value.toString();
    return true;
}

bool getNumber(QJsonValue value, double & out) {
    if(!value.isDouble()) {
        return false;
    }
    out = value.toDouble();
    return true;
}

/*
{
   "IssueInstant":"2020-12-07T19:52:08.4463796Z",
   "NotAfter":"2020-12-21T19:52:08.4463796Z",
   "Token":"token",
   "DisplayClaims":{
      "xui":[
         {
            "uhs":"userhash"
         }
      ]
   }
 }
*/
// TODO: handle error responses ...
/*
{
    "Identity":"0",
    "XErr":2148916238,
    "Message":"",
    "Redirect":"https://start.ui.xboxlive.com/AddChildToFamily"
}
// 2148916233 = missing XBox account
// 2148916238 = child account not linked to a family
*/

bool parseXTokenResponse(QByteArray & data, Katabasis::Token &output, const char * name) {
    qInfo() << "Parsing" << name <<":";
#ifndef NDEBUG
    qDebug() << data;
#endif
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
    if(jsonError.error) {
        qWarning() << "Failed to parse response from user.auth.xboxlive.com as JSON: " << jsonError.errorString();
        return false;
    }

    auto obj = doc.object();
    if(!getDateTime(obj.value("IssueInstant"), output.issueInstant)) {
        qWarning() << "User IssueInstant is not a timestamp";
        return false;
    }
    if(!getDateTime(obj.value("NotAfter"), output.notAfter)) {
        qWarning() << "User NotAfter is not a timestamp";
        return false;
    }
    if(!getString(obj.value("Token"), output.token)) {
        qWarning() << "User Token is not a timestamp";
        return false;
    }
    auto arrayVal = obj.value("DisplayClaims").toObject().value("xui");
    if(!arrayVal.isArray()) {
        qWarning() << "Missing xui claims array";
        return false;
    }
    bool foundUHS = false;
    for(auto item: arrayVal.toArray()) {
        if(!item.isObject()) {
            continue;
        }
        auto obj = item.toObject();
        if(obj.contains("uhs")) {
            foundUHS = true;
        } else {
            continue;
        }
        // consume all 'display claims' ... whatever that means
        for(auto iter = obj.begin(); iter != obj.end(); iter++) {
            QString claim;
            if(!getString(obj.value(iter.key()), claim)) {
                qWarning() << "display claim " << iter.key() << " is not a string...";
                return false;
            }
            output.extra[iter.key()] = claim;
        }

        break;
    }
    if(!foundUHS) {
        qWarning() << "Missing uhs";
        return false;
    }
    output.validity = Katabasis::Validity::Certain;
    qInfo() << name << "is valid.";
    return true;
}

}

void AuthContext::onUserAuthDone(
    int requestId,
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
    if(!parseXTokenResponse(replyData, temp, "UToken")) {
        qWarning() << "Could not parse user authentication response...";
        finishActivity();
        changeState(STATE_FAILED_HARD, tr("XBox user authentication response could not be understood."));
        return;
    }
    m_data->userToken = temp;

    m_stage = MSAStage::XboxAuth;
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
    Requestor *requestor = new Requestor(mgr, m_oauth2, this);
    requestor->setAddAccessTokenInQuery(false);

    connect(requestor, &Requestor::finished, this, &AuthContext::onSTSAuthMinecraftDone);
    requestor->post(request, xbox_auth_data.toUtf8());
    qDebug() << "Getting Minecraft services STS token...";
}

void AuthContext::onSTSAuthMinecraftDone(
    int requestId,
    QNetworkReply::NetworkError error,
    QByteArray replyData,
    QList<QNetworkReply::RawHeaderPair> headers
) {
    if (error != QNetworkReply::NoError) {
        qWarning() << "Reply error:" << error;
        m_requestsDone ++;
        return;
    }

    Katabasis::Token temp;
    if(!parseXTokenResponse(replyData, temp, "STSAuthMinecraft")) {
        qWarning() << "Could not parse authorization response for access to mojang services...";
        m_requestsDone ++;
        return;
    }

    if(temp.extra["uhs"] != m_data->userToken.extra["uhs"]) {
        qWarning() << "Server has changed user hash in the reply... something is wrong. ABORTING";
        qDebug() << replyData;
        m_requestsDone ++;
        return;
    }
    m_data->mojangservicesToken = temp;

    doMinecraftAuth();
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
    Requestor *requestor = new Requestor(mgr, m_oauth2, this);
    requestor->setAddAccessTokenInQuery(false);

    connect(requestor, &Requestor::finished, this, &AuthContext::onSTSAuthGenericDone);
    requestor->post(request, xbox_auth_data.toUtf8());
    qDebug() << "Getting generic STS token...";
}

void AuthContext::onSTSAuthGenericDone(
    int requestId,
    QNetworkReply::NetworkError error,
    QByteArray replyData,
    QList<QNetworkReply::RawHeaderPair> headers
) {
    if (error != QNetworkReply::NoError) {
        qWarning() << "Reply error:" << error;
        m_requestsDone ++;
        return;
    }

    Katabasis::Token temp;
    if(!parseXTokenResponse(replyData, temp, "STSAuthGaneric")) {
        qWarning() << "Could not parse authorization response for access to xbox API...";
        m_requestsDone ++;
        return;
    }

    if(temp.extra["uhs"] != m_data->userToken.extra["uhs"]) {
        qWarning() << "Server has changed user hash in the reply... something is wrong. ABORTING";
        qDebug() << replyData;
        m_requestsDone ++;
        return;
    }
    m_data->xboxApiToken = temp;

    doXBoxProfile();
}


void AuthContext::doMinecraftAuth() {
    QString mc_auth_template = R"XXX(
{
    "identityToken": "XBL3.0 x=%1;%2"
}
)XXX";
    auto data = mc_auth_template.arg(m_data->mojangservicesToken.extra["uhs"].toString(), m_data->mojangservicesToken.token);

    QNetworkRequest request = QNetworkRequest(QUrl("https://api.minecraftservices.com/authentication/login_with_xbox"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    Requestor *requestor = new Requestor(mgr, m_oauth2, this);
    requestor->setAddAccessTokenInQuery(false);

    connect(requestor, &Requestor::finished, this, &AuthContext::onMinecraftAuthDone);
    requestor->post(request, data.toUtf8());
    qDebug() << "Getting Minecraft access token...";
}

namespace {
bool parseMojangResponse(QByteArray & data, Katabasis::Token &output) {
    QJsonParseError jsonError;
    qDebug() << "Parsing Mojang response...";
#ifndef NDEBUG
    qDebug() << data;
#endif
    QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
    if(jsonError.error) {
        qWarning() << "Failed to parse response from api.minecraftservices.com/authentication/login_with_xbox as JSON: " << jsonError.errorString();
        return false;
    }

    auto obj = doc.object();
    double expires_in = 0;
    if(!getNumber(obj.value("expires_in"), expires_in)) {
        qWarning() << "expires_in is not a valid number";
        return false;
    }
    auto currentTime = QDateTime::currentDateTimeUtc();
    output.issueInstant = currentTime;
    output.notAfter = currentTime.addSecs(expires_in);

    QString username;
    if(!getString(obj.value("username"), username)) {
        qWarning() << "username is not valid";
        return false;
    }

    // TODO: it's a JWT... validate it?
    if(!getString(obj.value("access_token"), output.token)) {
        qWarning() << "access_token is not valid";
        return false;
    }
    output.validity = Katabasis::Validity::Certain;
    qDebug() << "Mojang response is valid.";
    return true;
}
}

void AuthContext::onMinecraftAuthDone(
    int requestId,
    QNetworkReply::NetworkError error,
    QByteArray replyData,
    QList<QNetworkReply::RawHeaderPair> headers
) {
    m_requestsDone ++;

    if (error != QNetworkReply::NoError) {
        qWarning() << "Reply error:" << error;
#ifndef NDEBUG
        qDebug() << replyData;
#endif
        return;
    }

    if(!parseMojangResponse(replyData, m_data->yggdrasilToken)) {
        qWarning() << "Could not parse login_with_xbox response...";
#ifndef NDEBUG
        qDebug() << replyData;
#endif
        return;
    }
    m_mcAuthSucceeded = true;

    checkResult();
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
    Requestor *requestor = new Requestor(mgr, m_oauth2, this);
    requestor->setAddAccessTokenInQuery(false);

    connect(requestor, &Requestor::finished, this, &AuthContext::onXBoxProfileDone);
    requestor->get(request);
    qDebug() << "Getting Xbox profile...";
}

void AuthContext::onXBoxProfileDone(
    int requestId,
    QNetworkReply::NetworkError error,
    QByteArray replyData,
    QList<QNetworkReply::RawHeaderPair> headers
) {
    m_requestsDone ++;

    if (error != QNetworkReply::NoError) {
        qWarning() << "Reply error:" << error;
#ifndef NDEBUG
        qDebug() << replyData;
#endif
        return;
    }

#ifndef NDEBUG
    qDebug() << "XBox profile: " << replyData;
#endif

    m_xboxProfileSucceeded = true;
    checkResult();
}

void AuthContext::checkResult() {
    if(m_requestsDone != 2) {
        return;
    }
    if(m_mcAuthSucceeded && m_xboxProfileSucceeded) {
        doMinecraftProfile();
    }
    else {
        finishActivity();
        changeState(STATE_FAILED_HARD, tr("XBox and/or Mojang authentication steps did not succeed"));
    }
}

namespace {
bool parseMinecraftProfile(QByteArray & data, MinecraftProfile &output) {
    qDebug() << "Parsing Minecraft profile...";
#ifndef NDEBUG
    qDebug() << data;
#endif

    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
    if(jsonError.error) {
        qWarning() << "Failed to parse response from user.auth.xboxlive.com as JSON: " << jsonError.errorString();
        return false;
    }

    auto obj = doc.object();
    if(!getString(obj.value("id"), output.id)) {
        qWarning() << "Minecraft profile id is not a string";
        return false;
    }

    if(!getString(obj.value("name"), output.name)) {
        qWarning() << "Minecraft profile name is not a string";
        return false;
    }

    auto skinsArray = obj.value("skins").toArray();
    for(auto skin: skinsArray) {
        auto skinObj = skin.toObject();
        Skin skinOut;
        if(!getString(skinObj.value("id"), skinOut.id)) {
            continue;
        }
        QString state;
        if(!getString(skinObj.value("state"), state)) {
            continue;
        }
        if(state != "ACTIVE") {
            continue;
        }
        if(!getString(skinObj.value("url"), skinOut.url)) {
            continue;
        }
        if(!getString(skinObj.value("variant"), skinOut.variant)) {
            continue;
        }
        // we deal with only the active skin
        output.skin = skinOut;
        break;
    }
    auto capesArray = obj.value("capes").toArray();
    int i = -1;
    int currentCape = -1;
    for(auto cape: capesArray) {
        i++;
        auto capeObj = cape.toObject();
        Cape capeOut;
        if(!getString(capeObj.value("id"), capeOut.id)) {
            continue;
        }
        QString state;
        if(!getString(capeObj.value("state"), state)) {
            continue;
        }
        if(state == "ACTIVE") {
            currentCape = i;
        }
        if(!getString(capeObj.value("url"), capeOut.url)) {
            continue;
        }
        if(!getString(capeObj.value("alias"), capeOut.alias)) {
            continue;
        }

        // we deal with only the active skin
        output.capes.push_back(capeOut);
    }
    output.currentCape = currentCape;
    output.validity = Katabasis::Validity::Certain;
    return true;
}
}

void AuthContext::doMinecraftProfile() {
    m_stage = MSAStage::MinecraftProfile;
    changeState(STATE_WORKING, tr("Starting minecraft profile acquisition"));

    auto url = QUrl("https://api.minecraftservices.com/minecraft/profile");
    QNetworkRequest request = QNetworkRequest(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    // request.setRawHeader("Accept", "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_data->yggdrasilToken.token).toUtf8());

    Requestor *requestor = new Requestor(mgr, m_oauth2, this);
    requestor->setAddAccessTokenInQuery(false);

    connect(requestor, &Requestor::finished, this, &AuthContext::onMinecraftProfileDone);
    requestor->get(request);
}

void AuthContext::onMinecraftProfileDone(int, QNetworkReply::NetworkError error, QByteArray data, QList<QNetworkReply::RawHeaderPair> headers) {
    qDebug() << data;
    if (error == QNetworkReply::ContentNotFoundError) {
        m_data->minecraftProfile = MinecraftProfile();
        finishActivity();
        changeState(STATE_FAILED_HARD, tr("Account is missing a profile"));
        return;
    }
    if (error != QNetworkReply::NoError) {
        finishActivity();
        changeState(STATE_FAILED_HARD, tr("Profile acquisition failed"));
        return;
    }
    if(!parseMinecraftProfile(data, m_data->minecraftProfile)) {
        m_data->minecraftProfile = MinecraftProfile();
        finishActivity();
        changeState(STATE_FAILED_HARD, tr("Profile response could not be parsed"));
        return;
    }
    doGetSkin();
}

void AuthContext::doGetSkin() {
    m_stage = MSAStage::Skin;
    changeState(STATE_WORKING, tr("Fetching player skin"));

    auto url = QUrl(m_data->minecraftProfile.skin.url);
    QNetworkRequest request = QNetworkRequest(url);
    Requestor *requestor = new Requestor(mgr, m_oauth2, this);
    requestor->setAddAccessTokenInQuery(false);
    connect(requestor, &Requestor::finished, this, &AuthContext::onSkinDone);
    requestor->get(request);
}

void AuthContext::onSkinDone(int, QNetworkReply::NetworkError error, QByteArray data, QList<QNetworkReply::RawHeaderPair>) {
    if (error == QNetworkReply::NoError) {
        m_data->minecraftProfile.skin.data = data;
    }
    m_data->validity_ = Katabasis::Validity::Certain;
    finishActivity();
    changeState(STATE_SUCCEEDED, tr("Finished all authentication steps"));
}

QString AuthContext::getStateMessage() const {
    switch (m_accountState)
    {
        case STATE_WORKING:
            switch(m_stage) {
                case MSAStage::Idle: {
                    QString loginMessage = tr("Logging in as %1 user");
                    if(m_data->type == AccountType::MSA) {
                        return loginMessage.arg("Microsoft");
                    }
                    else {
                        return loginMessage.arg("Mojang");
                    }
                }
                case MSAStage::UserAuth:
                    return tr("Logging in as XBox user");
                case MSAStage::XboxAuth:
                    return tr("Logging in with XBox and Mojang services");
                case MSAStage::MinecraftProfile:
                    return tr("Getting Minecraft profile");
                case MSAStage::Skin:
                    return tr("Getting Minecraft skin");
                default:
                    break;
            }
        default:
            return AccountTask::getStateMessage();
    }
}
