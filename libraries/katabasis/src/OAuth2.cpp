#include <QList>
#include <QPair>
#include <QDebug>
#include <QTcpServer>
#include <QMap>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QDateTime>
#include <QCryptographicHash>
#include <QTimer>
#include <QVariantMap>
#include <QUuid>
#include <QDataStream>

#include <QUrlQuery>

#include "katabasis/OAuth2.h"
#include "katabasis/PollServer.h"
#include "katabasis/ReplyServer.h"
#include "katabasis/Globals.h"

#include "JsonResponse.h"

namespace {
// ref: https://tools.ietf.org/html/rfc8628#section-3.2
// Exception: Google sign-in uses "verification_url" instead of "*_uri" - we'll accept both.
bool hasMandatoryDeviceAuthParams(const QVariantMap& params)
{
    if (!params.contains(Katabasis::OAUTH2_DEVICE_CODE))
        return false;

    if (!params.contains(Katabasis::OAUTH2_USER_CODE))
        return false;

    if (!(params.contains(Katabasis::OAUTH2_VERIFICATION_URI) || params.contains(Katabasis::OAUTH2_VERIFICATION_URL)))
        return false;

    if (!params.contains(Katabasis::OAUTH2_EXPIRES_IN))
        return false;

    return true;
}

QByteArray createQueryParameters(const QList<Katabasis::RequestParameter> &parameters) {
    QByteArray ret;
    bool first = true;
    for( auto & h: parameters) {
        if (first) {
            first = false;
        } else {
            ret.append("&");
        }
        ret.append(QUrl::toPercentEncoding(h.name) + "=" + QUrl::toPercentEncoding(h.value));
    }
    return ret;
}
}

namespace Katabasis {

OAuth2::OAuth2(Options & opts, Token & token, QObject *parent, QNetworkAccessManager *manager) : QObject(parent), token_(token) {
    manager_ = manager ? manager : new QNetworkAccessManager(this);
    grantFlow_ = GrantFlowAuthorizationCode;
    qRegisterMetaType<QNetworkReply::NetworkError>("QNetworkReply::NetworkError");
    options_ = opts;
}

bool OAuth2::linked() {
    return token_.validity != Validity::None;
}
void OAuth2::setLinked(bool v) {
    qDebug() << "OAuth2::setLinked:" << (v? "true": "false");
    token_.validity = v ? Validity::Certain : Validity::None;
}

QString OAuth2::token() {
    return token_.token;
}
void OAuth2::setToken(const QString &v) {
    token_.token = v;
}

QByteArray OAuth2::replyContent() const {
    return replyContent_;
}

void OAuth2::setReplyContent(const QByteArray &value) {
    replyContent_ = value;
    if (replyServer_) {
        replyServer_->setReplyContent(replyContent_);
    }
}

QVariantMap OAuth2::extraTokens() {
    return token_.extra;
}

void OAuth2::setExtraTokens(QVariantMap extraTokens) {
    token_.extra = extraTokens;
}

void OAuth2::setReplyServer(ReplyServer * server)
{
    delete replyServer_;

    replyServer_ = server;
    replyServer_->setReplyContent(replyContent_);
}

ReplyServer * OAuth2::replyServer() const
{
    return replyServer_;
}

void OAuth2::setPollServer(PollServer *server)
{
    if (pollServer_)
        pollServer_->deleteLater();

    pollServer_ = server;
}

PollServer *OAuth2::pollServer() const
{
    return pollServer_;
}

OAuth2::GrantFlow OAuth2::grantFlow() {
    return grantFlow_;
}

void OAuth2::setGrantFlow(OAuth2::GrantFlow value) {
    grantFlow_ = value;
}

QString OAuth2::username() {
    return username_;
}

void OAuth2::setUsername(const QString &value) {
    username_ = value;
}

QString OAuth2::password() {
    return password_;
}

void OAuth2::setPassword(const QString &value) {
    password_ = value;
}

QVariantMap OAuth2::extraRequestParams()
{
    return extraReqParams_;
}

void OAuth2::setExtraRequestParams(const QVariantMap &value)
{
    extraReqParams_ = value;
}

QString OAuth2::grantType()
{
    if (!grantType_.isEmpty())
        return grantType_;

    switch (grantFlow_) {
    case GrantFlowAuthorizationCode:
        return OAUTH2_GRANT_TYPE_CODE;
    case GrantFlowImplicit:
        return OAUTH2_GRANT_TYPE_TOKEN;
    case GrantFlowResourceOwnerPasswordCredentials:
        return OAUTH2_GRANT_TYPE_PASSWORD;
    case GrantFlowDevice:
        return OAUTH2_GRANT_TYPE_DEVICE;
    }

    return QString();
}

void OAuth2::setGrantType(const QString &value)
{
    grantType_ = value;
}

void OAuth2::updateActivity(Activity activity)
{
    if(activity_ != activity) {
        activity_ = activity;
        emit activityChanged(activity_);
    }
}

void OAuth2::link() {
    qDebug() << "OAuth2::link";

    // Create the reply server if it doesn't exist
    if(replyServer() == NULL) {
        ReplyServer * replyServer = new ReplyServer(this);
        connect(replyServer, &ReplyServer::verificationReceived, this, &OAuth2::onVerificationReceived);
        connect(replyServer, &ReplyServer::serverClosed, this, &OAuth2::serverHasClosed);
        setReplyServer(replyServer);
    }

    if (linked()) {
        qDebug() << "OAuth2::link: Linked already";
        emit linkingSucceeded();
        return;
    }

    setLinked(false);
    setToken("");
    setExtraTokens(QVariantMap());
    setRefreshToken(QString());
    setExpires(QDateTime());

    if (grantFlow_ == GrantFlowAuthorizationCode || grantFlow_ == GrantFlowImplicit) {

        QString uniqueState = QUuid::createUuid().toString().remove(QRegExp("([^a-zA-Z0-9]|[-])"));

        // FIXME: this should be part of a 'redirection handler' that would get injected into O2
        {
            quint16 foundPort = 0;
            // Start listening to authentication replies
            if (!replyServer()->isListening()) {
                auto ports = options_.listenerPorts;
                for(auto & port: ports) {
                    if (replyServer()->listen(QHostAddress::Any, port)) {
                        foundPort = replyServer()->serverPort();
                        qDebug() << "OAuth2::link: Reply server listening on port " << foundPort;
                        break;
                    }
                }
                if(foundPort == 0) {
                    qWarning() << "OAuth2::link: Reply server failed to start listening on any port out of " << ports;
                    emit linkingFailed();
                    return;
                }
            }

            // Save redirect URI, as we have to reuse it when requesting the access token
            redirectUri_ = options_.redirectionUrl.arg(foundPort);
            replyServer()->setUniqueState(uniqueState);
        }

        // Assemble intial authentication URL
        QUrl url(options_.authorizationUrl);
        QUrlQuery query(url);
        QList<QPair<QString, QString> > parameters;
        query.addQueryItem(OAUTH2_RESPONSE_TYPE, (grantFlow_ == GrantFlowAuthorizationCode)? OAUTH2_GRANT_TYPE_CODE: OAUTH2_GRANT_TYPE_TOKEN);
        query.addQueryItem(OAUTH2_CLIENT_ID, options_.clientIdentifier);
        query.addQueryItem(OAUTH2_REDIRECT_URI, redirectUri_);
        query.addQueryItem(OAUTH2_SCOPE, options_.scope.replace( " ", "+" ));
        query.addQueryItem(OAUTH2_STATE, uniqueState);
        if (!apiKey_.isEmpty()) {
            query.addQueryItem(OAUTH2_API_KEY, apiKey_);
        }
        for(auto iter = extraReqParams_.begin(); iter != extraReqParams_.end(); iter++) {
            query.addQueryItem(iter.key(), iter.value().toString());
        }
        url.setQuery(query);

        // Show authentication URL with a web browser
        qDebug() << "OAuth2::link: Emit openBrowser" << url.toString();
        emit openBrowser(url);
        updateActivity(Activity::LoggingIn);
    } else if (grantFlow_ == GrantFlowResourceOwnerPasswordCredentials) {
        QList<RequestParameter> parameters;
        parameters.append(RequestParameter(OAUTH2_CLIENT_ID, options_.clientIdentifier.toUtf8()));
        if ( !options_.clientSecret.isEmpty() ) {
            parameters.append(RequestParameter(OAUTH2_CLIENT_SECRET, options_.clientSecret.toUtf8()));
        }
        parameters.append(RequestParameter(OAUTH2_USERNAME, username_.toUtf8()));
        parameters.append(RequestParameter(OAUTH2_PASSWORD, password_.toUtf8()));
        parameters.append(RequestParameter(OAUTH2_GRANT_TYPE, OAUTH2_GRANT_TYPE_PASSWORD));
        parameters.append(RequestParameter(OAUTH2_SCOPE, options_.scope.toUtf8()));
        if ( !apiKey_.isEmpty() )
            parameters.append(RequestParameter(OAUTH2_API_KEY, apiKey_.toUtf8()));
        foreach (QString key, extraRequestParams().keys()) {
            parameters.append(RequestParameter(key.toUtf8(), extraRequestParams().value(key).toByteArray()));
        }
        QByteArray payload = createQueryParameters(parameters);

        qDebug() << "OAuth2::link: Sending token request for resource owner flow";
        QUrl url(options_.accessTokenUrl);
        QNetworkRequest tokenRequest(url);
        tokenRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        QNetworkReply *tokenReply = manager_->post(tokenRequest, payload);

        connect(tokenReply, SIGNAL(finished()), this, SLOT(onTokenReplyFinished()), Qt::QueuedConnection);
        connect(tokenReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onTokenReplyError(QNetworkReply::NetworkError)), Qt::QueuedConnection);
        updateActivity(Activity::LoggingIn);
    }
    else if (grantFlow_ == GrantFlowDevice) {
        QList<RequestParameter> parameters;
        parameters.append(RequestParameter(OAUTH2_CLIENT_ID, options_.clientIdentifier.toUtf8()));
        parameters.append(RequestParameter(OAUTH2_SCOPE, options_.scope.toUtf8()));
        QByteArray payload = createQueryParameters(parameters);

        QUrl url(options_.authorizationUrl);
        QNetworkRequest deviceRequest(url);
        deviceRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        QNetworkReply *tokenReply = manager_->post(deviceRequest, payload);

        connect(tokenReply, SIGNAL(finished()), this, SLOT(onDeviceAuthReplyFinished()), Qt::QueuedConnection);
        connect(tokenReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onTokenReplyError(QNetworkReply::NetworkError)), Qt::QueuedConnection);
        updateActivity(Activity::LoggingIn);
    }
}

void OAuth2::unlink() {
    qDebug() << "OAuth2::unlink";
    updateActivity(Activity::LoggingOut);
    // FIXME: implement logout flows... if they exist
    token_ = Token();
    updateActivity(Activity::Idle);
}

void OAuth2::onVerificationReceived(const QMap<QString, QString> response) {
    qDebug() << "OAuth2::onVerificationReceived: Emitting closeBrowser()";
    emit closeBrowser();

    if (response.contains("error")) {
        qWarning() << "OAuth2::onVerificationReceived: Verification failed:" << response;
        emit linkingFailed();
        updateActivity(Activity::Idle);
        return;
    }

    if (grantFlow_ == GrantFlowAuthorizationCode) {
        // NOTE: access code is temporary and should never be saved anywhere!
        auto access_code = response.value(QString(OAUTH2_GRANT_TYPE_CODE));

        // Exchange access code for access/refresh tokens
        QString query;
        if(!apiKey_.isEmpty())
            query = QString("?" + QString(OAUTH2_API_KEY) + "=" + apiKey_);
        QNetworkRequest tokenRequest(QUrl(options_.accessTokenUrl.toString() + query));
        tokenRequest.setHeader(QNetworkRequest::ContentTypeHeader, MIME_TYPE_XFORM);
        tokenRequest.setRawHeader("Accept", MIME_TYPE_JSON);
        QMap<QString, QString> parameters;
        parameters.insert(OAUTH2_GRANT_TYPE_CODE, access_code);
        parameters.insert(OAUTH2_CLIENT_ID, options_.clientIdentifier);
        if ( !options_.clientSecret.isEmpty() ) {
            parameters.insert(OAUTH2_CLIENT_SECRET, options_.clientSecret);
        }
        parameters.insert(OAUTH2_REDIRECT_URI, redirectUri_);
        parameters.insert(OAUTH2_GRANT_TYPE, AUTHORIZATION_CODE);
        QByteArray data = buildRequestBody(parameters);

        qDebug() << QString("OAuth2::onVerificationReceived: Exchange access code data:\n%1").arg(QString(data));

        QNetworkReply *tokenReply = manager_->post(tokenRequest, data);
        timedReplies_.add(tokenReply);
        connect(tokenReply, SIGNAL(finished()), this, SLOT(onTokenReplyFinished()), Qt::QueuedConnection);
        connect(tokenReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onTokenReplyError(QNetworkReply::NetworkError)), Qt::QueuedConnection);
    } else if (grantFlow_ == GrantFlowImplicit || grantFlow_ == GrantFlowDevice) {
        // Check for mandatory tokens
        if (response.contains(OAUTH2_ACCESS_TOKEN)) {
            qDebug() << "OAuth2::onVerificationReceived: Access token returned for implicit or device flow";
            setToken(response.value(OAUTH2_ACCESS_TOKEN));
            if (response.contains(OAUTH2_EXPIRES_IN)) {
                bool ok = false;
                int expiresIn = response.value(OAUTH2_EXPIRES_IN).toInt(&ok);
                if (ok) {
                    qDebug() << "OAuth2::onVerificationReceived: Token expires in" << expiresIn << "seconds";
                    setExpires(QDateTime::currentDateTimeUtc().addSecs(expiresIn));
                }
            }
            if (response.contains(OAUTH2_REFRESH_TOKEN)) {
                setRefreshToken(response.value(OAUTH2_REFRESH_TOKEN));
            }
            setLinked(true);
            emit linkingSucceeded();
        } else {
            qWarning() << "OAuth2::onVerificationReceived: Access token missing from response for implicit or device flow";
            emit linkingFailed();
        }
        updateActivity(Activity::Idle);
    } else {
        setToken(response.value(OAUTH2_ACCESS_TOKEN));
        setRefreshToken(response.value(OAUTH2_REFRESH_TOKEN));
        updateActivity(Activity::Idle);
    }
}

void OAuth2::onTokenReplyFinished() {
    qDebug() << "OAuth2::onTokenReplyFinished";
    QNetworkReply *tokenReply = qobject_cast<QNetworkReply *>(sender());
    if (!tokenReply)
    {
        qDebug() << "OAuth2::onTokenReplyFinished: reply is null";
        return;
    }
    if (tokenReply->error() == QNetworkReply::NoError) {
        QByteArray replyData = tokenReply->readAll();

        // Dump replyData
        // SENSITIVE DATA in RelWithDebInfo or Debug builds
        //qDebug() << "OAuth2::onTokenReplyFinished: replyData\n";
        //qDebug() << QString( replyData );

        QVariantMap tokens = parseJsonResponse(replyData);

        // Dump tokens
        qDebug() << "OAuth2::onTokenReplyFinished: Tokens returned:\n";
        foreach (QString key, tokens.keys()) {
            // SENSITIVE DATA in RelWithDebInfo or Debug builds, so it is truncated first
            qDebug() << key << ": "<< tokens.value( key ).toString();
        }

        // Check for mandatory tokens
        if (tokens.contains(OAUTH2_ACCESS_TOKEN)) {
            qDebug() << "OAuth2::onTokenReplyFinished: Access token returned";
            setToken(tokens.take(OAUTH2_ACCESS_TOKEN).toString());
            bool ok = false;
            int expiresIn = tokens.take(OAUTH2_EXPIRES_IN).toInt(&ok);
            if (ok) {
                qDebug() << "OAuth2::onTokenReplyFinished: Token expires in" << expiresIn << "seconds";
                setExpires(QDateTime::currentDateTimeUtc().addSecs(expiresIn));
            }
            setRefreshToken(tokens.take(OAUTH2_REFRESH_TOKEN).toString());
            setExtraTokens(tokens);
            timedReplies_.remove(tokenReply);
            setLinked(true);
            emit linkingSucceeded();
        } else {
            qWarning() << "OAuth2::onTokenReplyFinished: Access token missing from response";
            emit linkingFailed();
        }
    }
    tokenReply->deleteLater();
    updateActivity(Activity::Idle);
}

void OAuth2::onTokenReplyError(QNetworkReply::NetworkError error) {
    QNetworkReply *tokenReply = qobject_cast<QNetworkReply *>(sender());
    if (!tokenReply)
    {
        qDebug() << "OAuth2::onTokenReplyError: reply is null";
    } else {
        qWarning() << "OAuth2::onTokenReplyError: " << error << ": " << tokenReply->errorString();
        qDebug() << "OAuth2::onTokenReplyError: " << tokenReply->readAll();
        timedReplies_.remove(tokenReply);
    }

    setToken(QString());
    setRefreshToken(QString());
    emit linkingFailed();
}

QByteArray OAuth2::buildRequestBody(const QMap<QString, QString> &parameters) {
    QByteArray body;
    bool first = true;
    foreach (QString key, parameters.keys()) {
        if (first) {
            first = false;
        } else {
            body.append("&");
        }
        QString value = parameters.value(key);
        body.append(QUrl::toPercentEncoding(key) + QString("=").toUtf8() + QUrl::toPercentEncoding(value));
    }
    return body;
}

QDateTime OAuth2::expires() {
    return token_.notAfter;
}
void OAuth2::setExpires(QDateTime v) {
    token_.notAfter = v;
}

void OAuth2::startPollServer(const QVariantMap &params)
{
    bool ok = false;
    int expiresIn = params[OAUTH2_EXPIRES_IN].toInt(&ok);
    if (!ok) {
        qWarning() << "OAuth2::startPollServer: No expired_in parameter";
        emit linkingFailed();
        return;
    }

    qDebug() << "OAuth2::startPollServer: device_ and user_code expires in" << expiresIn << "seconds";

    QUrl url(options_.accessTokenUrl);
    QNetworkRequest authRequest(url);
    authRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    const QString deviceCode = params[OAUTH2_DEVICE_CODE].toString();
    const QString grantType = grantType_.isEmpty() ? OAUTH2_GRANT_TYPE_DEVICE : grantType_;

    QList<RequestParameter> parameters;
    parameters.append(RequestParameter(OAUTH2_CLIENT_ID, options_.clientIdentifier.toUtf8()));
    if ( !options_.clientSecret.isEmpty() ) {
        parameters.append(RequestParameter(OAUTH2_CLIENT_SECRET, options_.clientSecret.toUtf8()));
    }
    parameters.append(RequestParameter(OAUTH2_CODE, deviceCode.toUtf8()));
    parameters.append(RequestParameter(OAUTH2_GRANT_TYPE, grantType.toUtf8()));
    QByteArray payload = createQueryParameters(parameters);

    PollServer * pollServer = new PollServer(manager_, authRequest, payload, expiresIn, this);
    if (params.contains(OAUTH2_INTERVAL)) {
        int interval = params[OAUTH2_INTERVAL].toInt(&ok);
        if (ok)
            pollServer->setInterval(interval);
    }
    connect(pollServer, SIGNAL(verificationReceived(QMap<QString,QString>)), this, SLOT(onVerificationReceived(QMap<QString,QString>)));
    connect(pollServer, SIGNAL(serverClosed(bool)), this, SLOT(serverHasClosed(bool)));
    setPollServer(pollServer);
    pollServer->startPolling();
}

QString OAuth2::refreshToken() {
    return token_.refresh_token;
}
void OAuth2::setRefreshToken(const QString &v) {
#ifndef NDEBUG
    qDebug() << "OAuth2::setRefreshToken" << v << "...";
#endif
    token_.refresh_token = v;
}

bool OAuth2::refresh() {
    qDebug() << "OAuth2::refresh: Token: ..." << refreshToken().right(7);

    if (refreshToken().isEmpty()) {
        qWarning() << "OAuth2::refresh: No refresh token";
        onRefreshError(QNetworkReply::AuthenticationRequiredError);
        return false;
    }
    if (options_.accessTokenUrl.isEmpty()) {
        qWarning() << "OAuth2::refresh: Refresh token URL not set";
        onRefreshError(QNetworkReply::AuthenticationRequiredError);
        return false;
    }

    updateActivity(Activity::Refreshing);

    QNetworkRequest refreshRequest(options_.accessTokenUrl);
    refreshRequest.setHeader(QNetworkRequest::ContentTypeHeader, MIME_TYPE_XFORM);
    QMap<QString, QString> parameters;
    parameters.insert(OAUTH2_CLIENT_ID, options_.clientIdentifier);
    if ( !options_.clientSecret.isEmpty() ) {
        parameters.insert(OAUTH2_CLIENT_SECRET, options_.clientSecret);
    }
    parameters.insert(OAUTH2_REFRESH_TOKEN, refreshToken());
    parameters.insert(OAUTH2_GRANT_TYPE, OAUTH2_REFRESH_TOKEN);

    QByteArray data = buildRequestBody(parameters);
    QNetworkReply *refreshReply = manager_->post(refreshRequest, data);
    timedReplies_.add(refreshReply);
    connect(refreshReply, SIGNAL(finished()), this, SLOT(onRefreshFinished()), Qt::QueuedConnection);
    connect(refreshReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onRefreshError(QNetworkReply::NetworkError)), Qt::QueuedConnection);
    return true;
}

void OAuth2::onRefreshFinished() {
    QNetworkReply *refreshReply = qobject_cast<QNetworkReply *>(sender());

    if (refreshReply->error() == QNetworkReply::NoError) {
        QByteArray reply = refreshReply->readAll();
        QVariantMap tokens = parseJsonResponse(reply);
        setToken(tokens.value(OAUTH2_ACCESS_TOKEN).toString());
        setExpires(QDateTime::currentDateTimeUtc().addSecs(tokens.value(OAUTH2_EXPIRES_IN).toInt()));
        QString refreshToken = tokens.value(OAUTH2_REFRESH_TOKEN).toString();
        if(!refreshToken.isEmpty()) {
            setRefreshToken(refreshToken);
        }
        else {
            qDebug() << "No new refresh token. Keep the old one.";
        }
        timedReplies_.remove(refreshReply);
        setLinked(true);
        emit linkingSucceeded();
        emit refreshFinished(QNetworkReply::NoError);
        qDebug() << "New token expires in" << expires() << "seconds";
    } else {
        qDebug() << "OAuth2::onRefreshFinished: Error" << (int)refreshReply->error() << refreshReply->errorString();
    }
    refreshReply->deleteLater();
    updateActivity(Activity::Idle);
}

void OAuth2::onRefreshError(QNetworkReply::NetworkError error) {
    QNetworkReply *refreshReply = qobject_cast<QNetworkReply *>(sender());
    qWarning() << "OAuth2::onRefreshError: " << error;
    unlink();
    timedReplies_.remove(refreshReply);
    emit refreshFinished(error);
}

void OAuth2::onDeviceAuthReplyFinished()
{
    qDebug() << "OAuth2::onDeviceAuthReplyFinished";
    QNetworkReply *tokenReply = qobject_cast<QNetworkReply *>(sender());
    if (!tokenReply)
    {
      qDebug() << "OAuth2::onDeviceAuthReplyFinished: reply is null";
      return;
    }
    if (tokenReply->error() == QNetworkReply::NoError) {
        QByteArray replyData = tokenReply->readAll();

        // Dump replyData
        // SENSITIVE DATA in RelWithDebInfo or Debug builds
        //qDebug() << "OAuth2::onDeviceAuthReplyFinished: replyData\n";
        //qDebug() << QString( replyData );

        QVariantMap params = parseJsonResponse(replyData);

        // Dump tokens
        qDebug() << "OAuth2::onDeviceAuthReplyFinished: Tokens returned:\n";
        foreach (QString key, params.keys()) {
            // SENSITIVE DATA in RelWithDebInfo or Debug builds, so it is truncated first
            qDebug() << key << ": "<< params.value( key ).toString();
        }

        // Check for mandatory parameters
        if (hasMandatoryDeviceAuthParams(params)) {
            qDebug() << "OAuth2::onDeviceAuthReplyFinished: Device auth request response";

            const QString userCode = params.take(OAUTH2_USER_CODE).toString();
            QUrl uri = params.take(OAUTH2_VERIFICATION_URI).toUrl();
            if (uri.isEmpty())
                uri = params.take(OAUTH2_VERIFICATION_URL).toUrl();

            if (params.contains(OAUTH2_VERIFICATION_URI_COMPLETE))
                emit openBrowser(params.take(OAUTH2_VERIFICATION_URI_COMPLETE).toUrl());

            emit showVerificationUriAndCode(uri, userCode);

            startPollServer(params);
        } else {
            qWarning() << "OAuth2::onDeviceAuthReplyFinished: Mandatory parameters missing from response";
            emit linkingFailed();
            updateActivity(Activity::Idle);
        }
    }
    tokenReply->deleteLater();
}

void OAuth2::serverHasClosed(bool paramsfound)
{
    if ( !paramsfound ) {
        // server has probably timed out after receiving first response
        emit linkingFailed();
    }
    // poll server is not re-used for later auth requests
    setPollServer(NULL);
}

QString OAuth2::apiKey() {
    return apiKey_;
}

void OAuth2::setApiKey(const QString &value) {
    apiKey_ = value;
}

bool OAuth2::ignoreSslErrors() {
    return timedReplies_.ignoreSslErrors();
}

void OAuth2::setIgnoreSslErrors(bool ignoreSslErrors) {
    timedReplies_.setIgnoreSslErrors(ignoreSslErrors);
}

}
