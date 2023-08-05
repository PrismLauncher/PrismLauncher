#include <QCryptographicHash>
#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QList>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPair>
#include <QTcpServer>
#include <QTimer>
#include <QUuid>
#include <QVariantMap>

#include <QUrlQuery>

#include "katabasis/DeviceFlow.h"
#include "katabasis/Globals.h"
#include "katabasis/PollServer.h"

#include "JsonResponse.h"
#include "KatabasisLogging.h"

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

QByteArray createQueryParameters(const QList<Katabasis::RequestParameter>& parameters)
{
    QByteArray ret;
    bool first = true;
    for (auto& h : parameters) {
        if (first) {
            first = false;
        } else {
            ret.append("&");
        }
        ret.append(QUrl::toPercentEncoding(h.name) + "=" + QUrl::toPercentEncoding(h.value));
    }
    return ret;
}
}  // namespace

namespace Katabasis {

DeviceFlow::DeviceFlow(Options& opts, Token& token, QObject* parent, QNetworkAccessManager* manager) : QObject(parent), token_(token)
{
    manager_ = manager ? manager : new QNetworkAccessManager(this);
    qRegisterMetaType<QNetworkReply::NetworkError>("QNetworkReply::NetworkError");
    options_ = opts;
}

bool DeviceFlow::linked()
{
    return token_.validity != Validity::None;
}
void DeviceFlow::setLinked(bool v)
{
    qDebug() << "DeviceFlow::setLinked:" << (v ? "true" : "false");
    token_.validity = v ? Validity::Certain : Validity::None;
}

void DeviceFlow::updateActivity(Activity activity)
{
    if (activity_ == activity) {
        return;
    }

    activity_ = activity;
    switch (activity) {
        case Katabasis::Activity::Idle:
        case Katabasis::Activity::LoggingIn:
        case Katabasis::Activity::LoggingOut:
        case Katabasis::Activity::Refreshing:
            // non-terminal states...
            break;
        case Katabasis::Activity::FailedSoft:
            // terminal state, tokens did not change
            break;
        case Katabasis::Activity::FailedHard:
        case Katabasis::Activity::FailedGone:
            // terminal state, tokens are invalid
            token_ = Token();
            break;
        case Katabasis::Activity::Succeeded:
            setLinked(true);
            break;
    }
    emit activityChanged(activity_);
}

QString DeviceFlow::token()
{
    return token_.token;
}
void DeviceFlow::setToken(const QString& v)
{
    token_.token = v;
}

QVariantMap DeviceFlow::extraTokens()
{
    return token_.extra;
}

void DeviceFlow::setExtraTokens(QVariantMap extraTokens)
{
    token_.extra = extraTokens;
}

void DeviceFlow::setPollServer(PollServer* server)
{
    if (pollServer_)
        pollServer_->deleteLater();

    pollServer_ = server;
}

PollServer* DeviceFlow::pollServer() const
{
    return pollServer_;
}

QVariantMap DeviceFlow::extraRequestParams()
{
    return extraReqParams_;
}

void DeviceFlow::setExtraRequestParams(const QVariantMap& value)
{
    extraReqParams_ = value;
}

QString DeviceFlow::grantType()
{
    if (!grantType_.isEmpty())
        return grantType_;

    return OAUTH2_GRANT_TYPE_DEVICE;
}

void DeviceFlow::setGrantType(const QString& value)
{
    grantType_ = value;
}

// First get the URL and token to display to the user
void DeviceFlow::login()
{
    qDebug() << "DeviceFlow::link";

    updateActivity(Activity::LoggingIn);
    setLinked(false);
    setToken("");
    setExtraTokens(QVariantMap());
    setRefreshToken(QString());
    setExpires(QDateTime());

    QList<RequestParameter> parameters;
    parameters.append(RequestParameter(OAUTH2_CLIENT_ID, options_.clientIdentifier.toUtf8()));
    parameters.append(RequestParameter(OAUTH2_SCOPE, options_.scope.toUtf8()));
    QByteArray payload = createQueryParameters(parameters);

    QUrl url(options_.authorizationUrl);
    QNetworkRequest deviceRequest(url);
    deviceRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply* tokenReply = manager_->post(deviceRequest, payload);

    connect(tokenReply, &QNetworkReply::finished, this, &DeviceFlow::onDeviceAuthReplyFinished, Qt::QueuedConnection);
}

// Then, once we get them, present them to the user
void DeviceFlow::onDeviceAuthReplyFinished()
{
    qDebug() << "DeviceFlow::onDeviceAuthReplyFinished";
    QNetworkReply* tokenReply = qobject_cast<QNetworkReply*>(sender());
    if (!tokenReply) {
        qDebug() << "DeviceFlow::onDeviceAuthReplyFinished: reply is null";
        return;
    }
    if (tokenReply->error() == QNetworkReply::NoError) {
        QByteArray replyData = tokenReply->readAll();

        // Dump replyData
        // SENSITIVE DATA in RelWithDebInfo or Debug builds
        // qDebug() << "DeviceFlow::onDeviceAuthReplyFinished: replyData\n";
        // qDebug() << QString( replyData );

        QVariantMap params = parseJsonResponse(replyData);

        // Dump tokens
        qDebug() << "DeviceFlow::onDeviceAuthReplyFinished: Tokens returned:\n";
        foreach (QString key, params.keys()) {
            // SENSITIVE DATA in RelWithDebInfo or Debug builds, so it is truncated first
            qDebug() << key << ": " << params.value(key).toString();
        }

        // Check for mandatory parameters
        if (hasMandatoryDeviceAuthParams(params)) {
            qDebug() << "DeviceFlow::onDeviceAuthReplyFinished: Device auth request response";

            const QString userCode = params.take(OAUTH2_USER_CODE).toString();
            QUrl uri = params.take(OAUTH2_VERIFICATION_URI).toUrl();
            if (uri.isEmpty())
                uri = params.take(OAUTH2_VERIFICATION_URL).toUrl();

            if (params.contains(OAUTH2_VERIFICATION_URI_COMPLETE))
                emit openBrowser(params.take(OAUTH2_VERIFICATION_URI_COMPLETE).toUrl());

            bool ok = false;
            int expiresIn = params[OAUTH2_EXPIRES_IN].toInt(&ok);
            if (!ok) {
                qWarning() << "DeviceFlow::startPollServer: No expired_in parameter";
                updateActivity(Activity::FailedHard);
                return;
            }

            emit showVerificationUriAndCode(uri, userCode, expiresIn);

            startPollServer(params, expiresIn);
        } else {
            qWarning() << "DeviceFlow::onDeviceAuthReplyFinished: Mandatory parameters missing from response";
            updateActivity(Activity::FailedHard);
        }
    }
    tokenReply->deleteLater();
}

// Spin up polling for the user completing the login flow out of band
void DeviceFlow::startPollServer(const QVariantMap& params, int expiresIn)
{
    qDebug() << "DeviceFlow::startPollServer: device_ and user_code expires in" << expiresIn << "seconds";

    QUrl url(options_.accessTokenUrl);
    QNetworkRequest authRequest(url);
    authRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    const QString deviceCode = params[OAUTH2_DEVICE_CODE].toString();
    const QString grantType = grantType_.isEmpty() ? OAUTH2_GRANT_TYPE_DEVICE : grantType_;

    QList<RequestParameter> parameters;
    parameters.append(RequestParameter(OAUTH2_CLIENT_ID, options_.clientIdentifier.toUtf8()));
    if (!options_.clientSecret.isEmpty()) {
        parameters.append(RequestParameter(OAUTH2_CLIENT_SECRET, options_.clientSecret.toUtf8()));
    }
    parameters.append(RequestParameter(OAUTH2_CODE, deviceCode.toUtf8()));
    parameters.append(RequestParameter(OAUTH2_GRANT_TYPE, grantType.toUtf8()));
    QByteArray payload = createQueryParameters(parameters);

    PollServer* pollServer = new PollServer(manager_, authRequest, payload, expiresIn, this);
    if (params.contains(OAUTH2_INTERVAL)) {
        bool ok = false;
        int interval = params[OAUTH2_INTERVAL].toInt(&ok);
        if (ok) {
            pollServer->setInterval(interval);
        }
    }
    connect(pollServer, &PollServer::verificationReceived, this, &DeviceFlow::onVerificationReceived);
    connect(pollServer, &PollServer::serverClosed, this, &DeviceFlow::serverHasClosed);
    setPollServer(pollServer);
    pollServer->startPolling();
}

// Once the user completes the flow, update the internal state and report it to observers
void DeviceFlow::onVerificationReceived(const QMap<QString, QString> response)
{
    qDebug() << "DeviceFlow::onVerificationReceived: Emitting closeBrowser()";
    emit closeBrowser();

    if (response.contains("error")) {
        qWarning() << "DeviceFlow::onVerificationReceived: Verification failed:" << response;
        updateActivity(Activity::FailedHard);
        return;
    }

    // Check for mandatory tokens
    if (response.contains(OAUTH2_ACCESS_TOKEN)) {
        qDebug() << "DeviceFlow::onVerificationReceived: Access token returned for implicit or device flow";
        setToken(response.value(OAUTH2_ACCESS_TOKEN));
        if (response.contains(OAUTH2_EXPIRES_IN)) {
            bool ok = false;
            int expiresIn = response.value(OAUTH2_EXPIRES_IN).toInt(&ok);
            if (ok) {
                qDebug() << "DeviceFlow::onVerificationReceived: Token expires in" << expiresIn << "seconds";
                setExpires(QDateTime::currentDateTimeUtc().addSecs(expiresIn));
            }
        }
        if (response.contains(OAUTH2_REFRESH_TOKEN)) {
            setRefreshToken(response.value(OAUTH2_REFRESH_TOKEN));
        }
        updateActivity(Activity::Succeeded);
    } else {
        qWarning() << "DeviceFlow::onVerificationReceived: Access token missing from response for implicit or device flow";
        updateActivity(Activity::FailedHard);
    }
}

// Or if the flow fails or the polling times out, update the internal state with error and report it to observers
void DeviceFlow::serverHasClosed(bool paramsfound)
{
    if (!paramsfound) {
        // server has probably timed out after receiving first response
        updateActivity(Activity::FailedHard);
    }
    // poll server is not re-used for later auth requests
    setPollServer(NULL);
}

void DeviceFlow::logout()
{
    qDebug() << "DeviceFlow::unlink";
    updateActivity(Activity::LoggingOut);
    // FIXME: implement logout flows... if they exist
    token_ = Token();
    updateActivity(Activity::FailedHard);
}

QDateTime DeviceFlow::expires()
{
    return token_.notAfter;
}
void DeviceFlow::setExpires(QDateTime v)
{
    token_.notAfter = v;
}

QString DeviceFlow::refreshToken()
{
    return token_.refresh_token;
}

void DeviceFlow::setRefreshToken(const QString& v)
{
    qCDebug(katabasisCredentials) << "new refresh token:" << v;
    token_.refresh_token = v;
}

namespace {
QByteArray buildRequestBody(const QMap<QString, QString>& parameters)
{
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
}  // namespace

bool DeviceFlow::refresh()
{
    qDebug() << "DeviceFlow::refresh: Token: ..." << refreshToken().right(7);

    updateActivity(Activity::Refreshing);

    if (refreshToken().isEmpty()) {
        qWarning() << "DeviceFlow::refresh: No refresh token";
        onRefreshError(QNetworkReply::AuthenticationRequiredError, nullptr);
        return false;
    }
    if (options_.accessTokenUrl.isEmpty()) {
        qWarning() << "DeviceFlow::refresh: Refresh token URL not set";
        onRefreshError(QNetworkReply::AuthenticationRequiredError, nullptr);
        return false;
    }

    QNetworkRequest refreshRequest(options_.accessTokenUrl);
    refreshRequest.setHeader(QNetworkRequest::ContentTypeHeader, MIME_TYPE_XFORM);
    QMap<QString, QString> parameters;
    parameters.insert(OAUTH2_CLIENT_ID, options_.clientIdentifier);
    if (!options_.clientSecret.isEmpty()) {
        parameters.insert(OAUTH2_CLIENT_SECRET, options_.clientSecret);
    }
    parameters.insert(OAUTH2_REFRESH_TOKEN, refreshToken());
    parameters.insert(OAUTH2_GRANT_TYPE, OAUTH2_REFRESH_TOKEN);

    QByteArray data = buildRequestBody(parameters);
    QNetworkReply* refreshReply = manager_->post(refreshRequest, data);
    timedReplies_.add(refreshReply);
    connect(refreshReply, &QNetworkReply::finished, this, &DeviceFlow::onRefreshFinished, Qt::QueuedConnection);
    return true;
}

void DeviceFlow::onRefreshFinished()
{
    QNetworkReply* refreshReply = qobject_cast<QNetworkReply*>(sender());

    auto networkError = refreshReply->error();
    if (networkError == QNetworkReply::NoError) {
        QByteArray reply = refreshReply->readAll();
        QVariantMap tokens = parseJsonResponse(reply);
        setToken(tokens.value(OAUTH2_ACCESS_TOKEN).toString());
        setExpires(QDateTime::currentDateTimeUtc().addSecs(tokens.value(OAUTH2_EXPIRES_IN).toInt()));
        QString refreshToken = tokens.value(OAUTH2_REFRESH_TOKEN).toString();
        if (!refreshToken.isEmpty()) {
            setRefreshToken(refreshToken);
        } else {
            qDebug() << "No new refresh token. Keep the old one.";
        }
        timedReplies_.remove(refreshReply);
        refreshReply->deleteLater();
        updateActivity(Activity::Succeeded);
        qDebug() << "New token expires in" << expires() << "seconds";
    } else {
        // FIXME: differentiate the error more here
        onRefreshError(networkError, refreshReply);
    }
}

void DeviceFlow::onRefreshError(QNetworkReply::NetworkError error, QNetworkReply* refreshReply)
{
    QString errorString = "No Reply";
    if (refreshReply) {
        timedReplies_.remove(refreshReply);
        errorString = refreshReply->errorString();
    }

    switch (error) {
        // used for invalid credentials and similar errors. Fall through.
        case QNetworkReply::AuthenticationRequiredError:
        case QNetworkReply::ContentAccessDenied:
        case QNetworkReply::ContentOperationNotPermittedError:
        case QNetworkReply::ProtocolInvalidOperationError:
            updateActivity(Activity::FailedHard);
            break;
        case QNetworkReply::ContentGoneError: {
            updateActivity(Activity::FailedGone);
            break;
        }
        case QNetworkReply::TimeoutError:
        case QNetworkReply::OperationCanceledError:
        case QNetworkReply::SslHandshakeFailedError:
        default:
            updateActivity(Activity::FailedSoft);
            return;
    }
    if (refreshReply) {
        refreshReply->deleteLater();
    }
    qDebug() << "DeviceFlow::onRefreshFinished: Error" << static_cast<int>(error) << " - " << errorString;
}

}  // namespace Katabasis
