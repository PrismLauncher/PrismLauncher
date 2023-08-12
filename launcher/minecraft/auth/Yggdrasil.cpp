/* Copyright 2013-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Yggdrasil.h"
#include "AccountData.h"

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QObject>
#include <QString>

#include <QDebug>

#include "Application.h"

Yggdrasil::Yggdrasil(AccountData* data, QObject* parent) : AccountTask(data, parent)
{
    changeState(AccountTaskState::STATE_CREATED);
}

void Yggdrasil::sendRequest(QUrl endpoint, QByteArray content)
{
    changeState(AccountTaskState::STATE_WORKING);

    QNetworkRequest netRequest(endpoint);
    netRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    m_netReply = APPLICATION->network()->post(netRequest, content);
    connect(m_netReply, &QNetworkReply::finished, this, &Yggdrasil::processReply);
    connect(m_netReply, &QNetworkReply::uploadProgress, this, &Yggdrasil::refreshTimers);
    connect(m_netReply, &QNetworkReply::downloadProgress, this, &Yggdrasil::refreshTimers);
    connect(m_netReply, &QNetworkReply::sslErrors, this, &Yggdrasil::sslErrors);
    timeout_keeper.setSingleShot(true);
    timeout_keeper.start(timeout_max);
    counter.setSingleShot(false);
    counter.start(time_step);
    progress(0, timeout_max);
    connect(&timeout_keeper, &QTimer::timeout, this, &Yggdrasil::abortByTimeout);
    connect(&counter, &QTimer::timeout, this, &Yggdrasil::heartbeat);
}

void Yggdrasil::executeTask() {}

void Yggdrasil::refresh()
{
    start();
    /*
     * {
     *  "clientToken": "client identifier"
     *  "accessToken": "current access token to be refreshed"
     *  "selectedProfile":                      // specifying this causes errors
     *  {
     *   "id": "profile ID"
     *   "name": "profile name"
     *  }
     *  "requestUser": true/false               // request the user structure
     * }
     */
    QJsonObject req;
    req.insert("clientToken", m_data->clientToken());
    req.insert("accessToken", m_data->accessToken());
    /*
    {
        auto currentProfile = m_account->currentProfile();
        QJsonObject profile;
        profile.insert("id", currentProfile->id());
        profile.insert("name", currentProfile->name());
        req.insert("selectedProfile", profile);
    }
    */
    req.insert("requestUser", false);
    QJsonDocument doc(req);

    QUrl reqUrl("https://authserver.mojang.com/refresh");
    QByteArray requestData = doc.toJson();

    sendRequest(reqUrl, requestData);
}

void Yggdrasil::login(QString password)
{
    start();
    /*
     * {
     *   "agent": {                              // optional
     *   "name": "Minecraft",                    // So far this is the only encountered value
     *   "version": 1                            // This number might be increased
     *                                           // by the vanilla client in the future
     *   },
     *   "username": "mojang account name",      // Can be an email address or player name for
     *                                           // unmigrated accounts
     *   "password": "mojang account password",
     *   "clientToken": "client identifier",     // optional
     *   "requestUser": true/false               // request the user structure
     * }
     */
    QJsonObject req;

    {
        QJsonObject agent;
        // C++ makes string literals void* for some stupid reason, so we have to tell it
        // QString... Thanks Obama.
        agent.insert("name", QString("Minecraft"));
        agent.insert("version", 1);
        req.insert("agent", agent);
    }

    req.insert("username", m_data->userName());
    req.insert("password", password);
    req.insert("requestUser", false);

    // If we already have a client token, give it to the server.
    // Otherwise, let the server give us one.

    m_data->generateClientTokenIfMissing();
    req.insert("clientToken", m_data->clientToken());

    QJsonDocument doc(req);

    QUrl reqUrl("https://authserver.mojang.com/authenticate");
    QNetworkRequest netRequest(reqUrl);
    QByteArray requestData = doc.toJson();

    sendRequest(reqUrl, requestData);
}

void Yggdrasil::refreshTimers(qint64, qint64)
{
    timeout_keeper.stop();
    timeout_keeper.start(timeout_max);
    progress(count = 0, timeout_max);
}

void Yggdrasil::heartbeat()
{
    count += time_step;
    progress(count, timeout_max);
}

bool Yggdrasil::abort()
{
    progress(timeout_max, timeout_max);
    // TODO: actually use this in a meaningful way
    m_aborted = Yggdrasil::BY_USER;
    m_netReply->abort();
    return true;
}

void Yggdrasil::abortByTimeout()
{
    progress(timeout_max, timeout_max);
    // TODO: actually use this in a meaningful way
    m_aborted = Yggdrasil::BY_TIMEOUT;
    m_netReply->abort();
}

void Yggdrasil::sslErrors(QList<QSslError> errors)
{
    int i = 1;
    for (auto error : errors) {
        qCritical() << "LOGIN SSL Error #" << i << " : " << error.errorString();
        auto cert = error.certificate();
        qCritical() << "Certificate in question:\n" << cert.toText();
        i++;
    }
}

void Yggdrasil::processResponse(QJsonObject responseData)
{
    // Read the response data. We need to get the client token, access token, and the selected
    // profile.
    qDebug() << "Processing authentication response.";

    // qDebug() << responseData;
    // If we already have a client token, make sure the one the server gave us matches our
    // existing one.
    QString clientToken = responseData.value("clientToken").toString("");
    if (clientToken.isEmpty()) {
        // Fail if the server gave us an empty client token
        changeState(AccountTaskState::STATE_FAILED_HARD, tr("Authentication server didn't send a client token."));
        return;
    }
    if (m_data->clientToken().isEmpty()) {
        m_data->setClientToken(clientToken);
    } else if (clientToken != m_data->clientToken()) {
        changeState(AccountTaskState::STATE_FAILED_HARD,
                    tr("Authentication server attempted to change the client token. This isn't supported."));
        return;
    }

    // Now, we set the access token.
    qDebug() << "Getting access token.";
    QString accessToken = responseData.value("accessToken").toString("");
    if (accessToken.isEmpty()) {
        // Fail if the server didn't give us an access token.
        changeState(AccountTaskState::STATE_FAILED_HARD, tr("Authentication server didn't send an access token."));
        return;
    }
    // Set the access token.
    m_data->yggdrasilToken.token = accessToken;
    m_data->yggdrasilToken.validity = Katabasis::Validity::Certain;
    m_data->yggdrasilToken.issueInstant = QDateTime::currentDateTimeUtc();

    // Get UUID here since we need it for later
    auto profile = responseData.value("selectedProfile");
    if (!profile.isObject()) {
        changeState(AccountTaskState::STATE_FAILED_HARD, tr("Authentication server didn't send a selected profile."));
        return;
    }

    auto profileObj = profile.toObject();
    for (auto i = profileObj.constBegin(); i != profileObj.constEnd(); ++i) {
        if (i.key() == "name" && i.value().isString()) {
            m_data->minecraftProfile.name = i->toString();
        } else if (i.key() == "id" && i.value().isString()) {
            m_data->minecraftProfile.id = i->toString();
        }
    }

    if (m_data->minecraftProfile.id.isEmpty()) {
        changeState(AccountTaskState::STATE_FAILED_HARD, tr("Authentication server didn't send a UUID in selected profile."));
        return;
    }

    // We've made it through the minefield of possible errors. Return true to indicate that
    // we've succeeded.
    qDebug() << "Finished reading authentication response.";
    changeState(AccountTaskState::STATE_SUCCEEDED);
}

void Yggdrasil::processReply()
{
    changeState(AccountTaskState::STATE_WORKING);

    switch (m_netReply->error()) {
        case QNetworkReply::NoError:
            break;
        case QNetworkReply::TimeoutError:
            changeState(AccountTaskState::STATE_FAILED_SOFT, tr("Authentication operation timed out."));
            return;
        case QNetworkReply::OperationCanceledError:
            changeState(AccountTaskState::STATE_FAILED_SOFT, tr("Authentication operation cancelled."));
            return;
        case QNetworkReply::SslHandshakeFailedError:
            changeState(AccountTaskState::STATE_FAILED_SOFT,
                        tr("<b>SSL Handshake failed.</b><br/>There might be a few causes for it:<br/>"
                           "<ul>"
                           "<li>You use Windows and need to update your root certificates, please install any outstanding updates.</li>"
                           "<li>Some device on your network is interfering with SSL traffic. In that case, "
                           "you have bigger worries than Minecraft not starting.</li>"
                           "<li>Possibly something else. Check the log file for details</li>"
                           "</ul>"));
            return;
        // used for invalid credentials and similar errors. Fall through.
        case QNetworkReply::ContentAccessDenied:
        case QNetworkReply::ContentOperationNotPermittedError:
            break;
        case QNetworkReply::ContentGoneError: {
            changeState(AccountTaskState::STATE_FAILED_GONE,
                        tr("The Mojang account no longer exists. It may have been migrated to a Microsoft account."));
            return;
        }
        default:
            changeState(AccountTaskState::STATE_FAILED_SOFT, tr("Authentication operation failed due to a network error: %1 (%2)")
                                                                 .arg(m_netReply->errorString())
                                                                 .arg(m_netReply->error()));
            return;
    }

    // Try to parse the response regardless of the response code.
    // Sometimes the auth server will give more information and an error code.
    QJsonParseError jsonError;
    QByteArray replyData = m_netReply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(replyData, &jsonError);
    // Check the response code.
    int responseCode = m_netReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (responseCode == 200) {
        // If the response code was 200, then there shouldn't be an error. Make sure
        // anyways.
        // Also, sometimes an empty reply indicates success. If there was no data received,
        // pass an empty json object to the processResponse function.
        if (jsonError.error == QJsonParseError::NoError || replyData.size() == 0) {
            processResponse(replyData.size() > 0 ? doc.object() : QJsonObject());
            return;
        } else {
            changeState(AccountTaskState::STATE_FAILED_SOFT,
                        tr("Failed to parse authentication server response JSON response: %1 at offset %2.")
                            .arg(jsonError.errorString())
                            .arg(jsonError.offset));
            qCritical() << replyData;
        }
        return;
    }

    // If the response code was not 200, then Yggdrasil may have given us information
    // about the error.
    // If we can parse the response, then get information from it. Otherwise just say
    // there was an unknown error.
    if (jsonError.error == QJsonParseError::NoError) {
        // We were able to parse the server's response. Woo!
        // Call processError. If a subclass has overridden it then they'll handle their
        // stuff there.
        qDebug() << "The request failed, but the server gave us an error message. Processing error.";
        processError(doc.object());
    } else {
        // The server didn't say anything regarding the error. Give the user an unknown
        // error.
        qDebug() << "The request failed and the server gave no error message. Unknown error.";
        changeState(
            AccountTaskState::STATE_FAILED_SOFT,
            tr("An unknown error occurred when trying to communicate with the authentication server: %1").arg(m_netReply->errorString()));
    }
}

void Yggdrasil::processError(QJsonObject responseData)
{
    QJsonValue errorVal = responseData.value("error");
    QJsonValue errorMessageValue = responseData.value("errorMessage");
    QJsonValue causeVal = responseData.value("cause");

    if (errorVal.isString() && errorMessageValue.isString()) {
        m_error = std::shared_ptr<Error>(new Error{ errorVal.toString(""), errorMessageValue.toString(""), causeVal.toString("") });
        changeState(AccountTaskState::STATE_FAILED_HARD, m_error->m_errorMessageVerbose);
    } else {
        // Error is not in standard format. Don't set m_error and return unknown error.
        changeState(AccountTaskState::STATE_FAILED_HARD, tr("An unknown Yggdrasil error occurred."));
    }
}
