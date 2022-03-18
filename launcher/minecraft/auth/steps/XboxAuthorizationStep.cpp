#include "XboxAuthorizationStep.h"

#include <QNetworkRequest>
#include <QJsonParseError>
#include <QJsonDocument>

#include "minecraft/auth/AuthRequest.h"
#include "minecraft/auth/Parsers.h"

XboxAuthorizationStep::XboxAuthorizationStep(AccountData* data, Katabasis::Token *token, QString relyingParty, QString authorizationKind):
    AuthStep(data),
    m_token(token),
    m_relyingParty(relyingParty),
    m_authorizationKind(authorizationKind)
{
}

XboxAuthorizationStep::~XboxAuthorizationStep() noexcept = default;

QString XboxAuthorizationStep::describe() {
    return tr("Getting authorization to access %1 services.").arg(m_authorizationKind);
}

void XboxAuthorizationStep::rehydrate() {
    // FIXME: check if the tokens are good?
}

void XboxAuthorizationStep::perform() {
    QString xbox_auth_template = R"XXX(
{
    "Properties": {
        "SandboxId": "RETAIL",
        "UserTokens": [
            "%1"
        ]
    },
    "RelyingParty": "%2",
    "TokenType": "JWT"
}
)XXX";
    auto xbox_auth_data = xbox_auth_template.arg(m_data->userToken.token, m_relyingParty);
// http://xboxlive.com
    QNetworkRequest request = QNetworkRequest(QUrl("https://xsts.auth.xboxlive.com/xsts/authorize"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    AuthRequest *requestor = new AuthRequest(this);
    connect(requestor, &AuthRequest::finished, this, &XboxAuthorizationStep::onRequestDone);
    requestor->post(request, xbox_auth_data.toUtf8());
    qDebug() << "Getting authorization token for " << m_relyingParty;
}

void XboxAuthorizationStep::onRequestDone(
    QNetworkReply::NetworkError error,
    QByteArray data,
    QList<QNetworkReply::RawHeaderPair> headers
) {
    auto requestor = qobject_cast<AuthRequest *>(QObject::sender());
    requestor->deleteLater();

#ifndef NDEBUG
    qDebug() << data;
#endif
    if (error != QNetworkReply::NoError) {
        qWarning() << "Reply error:" << error;
        if(!processSTSError(error, data, headers)) {
            emit finished(
                AccountTaskState::STATE_FAILED_SOFT,
                tr("Failed to get authorization for %1 services. Error %2.").arg(m_authorizationKind, error)
            );
        }
        return;
    }

    Katabasis::Token temp;
    if(!Parsers::parseXTokenResponse(data, temp, m_authorizationKind)) {
        emit finished(
            AccountTaskState::STATE_FAILED_SOFT,
            tr("Could not parse authorization response for access to %1 services.").arg(m_authorizationKind)
        );
        return;
    }

    if(temp.extra["uhs"] != m_data->userToken.extra["uhs"]) {
        emit finished(
            AccountTaskState::STATE_FAILED_SOFT,
            tr("Server has changed %1 authorization user hash in the reply. Something is wrong.").arg(m_authorizationKind)
        );
        return;
    }
    auto & token = *m_token;
    token = temp;

    emit finished(AccountTaskState::STATE_WORKING, tr("Got authorization to access %1").arg(m_relyingParty));
}


bool XboxAuthorizationStep::processSTSError(
    QNetworkReply::NetworkError error,
    QByteArray data,
    QList<QNetworkReply::RawHeaderPair> headers
) {
    if(error == QNetworkReply::AuthenticationRequiredError) {
        QJsonParseError jsonError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
        if(jsonError.error) {
            qWarning() << "Cannot parse error XSTS response as JSON: " << jsonError.errorString();
            emit finished(
                AccountTaskState::STATE_FAILED_SOFT,
                tr("Cannot parse %1 authorization error response as JSON: %2").arg(m_authorizationKind, jsonError.errorString())
            );
            return true;
        }

        int64_t errorCode = -1;
        auto obj = doc.object();
        if(!Parsers::getNumber(obj.value("XErr"), errorCode)) {
            emit finished(
                AccountTaskState::STATE_FAILED_SOFT,
                tr("XErr element is missing from %1 authorization error response.").arg(m_authorizationKind)
            );
            return true;
        }
        switch(errorCode) {
            case 2148916233:{
                emit finished(
                    AccountTaskState::STATE_FAILED_SOFT,
                    tr("This Microsoft account does not have an XBox Live profile. Buy the game on %1 first.")
                        .arg("<a href=\"https://www.minecraft.net/en-us/store/minecraft-java-edition\">minecraft.net</a>")
                );
                return true;
            }
            case 2148916235: {
                // NOTE: this is the Grulovia error
                emit finished(
                    AccountTaskState::STATE_FAILED_SOFT,
                    tr("XBox Live is not available in your country. You've been blocked.")
                );
                return true;
            }
            case 2148916238: {
                emit finished(
                    AccountTaskState::STATE_FAILED_SOFT,
                    tr("This Microsoft account is underaged and is not linked to a family.\n\nPlease set up your account according to %1.")
                        .arg("<a href=\"https://help.minecraft.net/hc/en-us/articles/4403181904525\">help.minecraft.net</a>")
                );
                return true;
            }
            default: {
                emit finished(
                    AccountTaskState::STATE_FAILED_SOFT,
                    tr("XSTS authentication ended with unrecognized error(s):\n\n%1").arg(errorCode)
                );
                return true;
            }
        }
    }
    return false;
}
