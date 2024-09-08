#include "XboxAuthorizationStep.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QNetworkRequest>

#include "Application.h"
#include "Logging.h"
#include "minecraft/auth/Parsers.h"
#include "net/NetUtils.h"
#include "net/RawHeaderProxy.h"
#include "net/Upload.h"

XboxAuthorizationStep::XboxAuthorizationStep(AccountData* data, Token* token, QString relyingParty, QString authorizationKind)
    : AuthStep(data), m_token(token), m_relyingParty(relyingParty), m_authorizationKind(authorizationKind)
{}

QString XboxAuthorizationStep::describe()
{
    return tr("Getting authorization to access %1 services.").arg(m_authorizationKind);
}

void XboxAuthorizationStep::perform()
{
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
    QUrl url("https://xsts.auth.xboxlive.com/xsts/authorize");
    auto headers = QList<Net::HeaderPair>{
        { "Content-Type", "application/json" },
        { "Accept", "application/json" },
    };
    m_response.reset(new QByteArray());
    m_request = Net::Upload::makeByteArray(url, m_response, xbox_auth_data.toUtf8());
    m_request->addHeaderProxy(new Net::RawHeaderProxy(headers));

    m_task.reset(new NetJob("XboxAuthorizationStep", APPLICATION->network()));
    m_task->setAskRetry(false);
    m_task->addNetAction(m_request);

    connect(m_task.get(), &Task::finished, this, &XboxAuthorizationStep::onRequestDone);

    m_task->start();
    qDebug() << "Getting authorization token for " << m_relyingParty;
}

void XboxAuthorizationStep::onRequestDone()
{
    qCDebug(authCredentials()) << *m_response;
    if (m_request->error() != QNetworkReply::NoError) {
        qWarning() << "Reply error:" << m_request->error();
        if (Net::isApplicationError(m_request->error())) {
            if (!processSTSError()) {
                emit finished(AccountTaskState::STATE_FAILED_SOFT,
                              tr("Failed to get authorization for %1 services. Error %2.").arg(m_authorizationKind, m_request->error()));
            } else {
                emit finished(AccountTaskState::STATE_FAILED_SOFT,
                              tr("Unknown STS error for %1 services: %2").arg(m_authorizationKind, m_request->errorString()));
            }
        } else {
            emit finished(AccountTaskState::STATE_OFFLINE,
                          tr("Failed to get authorization for %1 services: %2").arg(m_authorizationKind, m_request->errorString()));
        }
        return;
    }

    Token temp;
    if (!Parsers::parseXTokenResponse(*m_response, temp, m_authorizationKind)) {
        emit finished(AccountTaskState::STATE_FAILED_SOFT,
                      tr("Could not parse authorization response for access to %1 services.").arg(m_authorizationKind));
        return;
    }

    if (temp.extra["uhs"] != m_data->userToken.extra["uhs"]) {
        emit finished(AccountTaskState::STATE_FAILED_SOFT,
                      tr("Server has changed %1 authorization user hash in the reply. Something is wrong.").arg(m_authorizationKind));
        return;
    }
    auto& token = *m_token;
    token = temp;

    emit finished(AccountTaskState::STATE_WORKING, tr("Got authorization to access %1").arg(m_relyingParty));
}

bool XboxAuthorizationStep::processSTSError()
{
    if (m_request->error() == QNetworkReply::AuthenticationRequiredError) {
        QJsonParseError jsonError;
        QJsonDocument doc = QJsonDocument::fromJson(*m_response, &jsonError);
        if (jsonError.error) {
            qWarning() << "Cannot parse error XSTS response as JSON: " << jsonError.errorString();
            emit finished(AccountTaskState::STATE_FAILED_SOFT,
                          tr("Cannot parse %1 authorization error response as JSON: %2").arg(m_authorizationKind, jsonError.errorString()));
            return true;
        }

        int64_t errorCode = -1;
        auto obj = doc.object();
        if (!Parsers::getNumber(obj.value("XErr"), errorCode)) {
            emit finished(AccountTaskState::STATE_FAILED_SOFT,
                          tr("XErr element is missing from %1 authorization error response.").arg(m_authorizationKind));
            return true;
        }
        switch (errorCode) {
            case 2148916233: {
                emit finished(AccountTaskState::STATE_FAILED_SOFT,
                              tr("This Microsoft account does not have an XBox Live profile. Buy the game on %1 first.")
                                  .arg("<a href=\"https://www.minecraft.net/en-us/store/minecraft-java-edition\">minecraft.net</a>"));
                return true;
            }
            case 2148916235: {
                // NOTE: this is the Grulovia error
                emit finished(AccountTaskState::STATE_FAILED_SOFT, tr("XBox Live is not available in your country. You've been blocked."));
                return true;
            }
            case 2148916238: {
                emit finished(
                    AccountTaskState::STATE_FAILED_SOFT,
                    tr("This Microsoft account is underaged and is not linked to a family.\n\nPlease set up your account according to %1.")
                        .arg("<a href=\"https://help.minecraft.net/hc/en-us/articles/4408968616077\">help.minecraft.net</a>"));
                return true;
            }
            // the following codes where copied from: https://github.com/PrismarineJS/prismarine-auth/pull/44
            case 2148916236: {
                emit finished(AccountTaskState::STATE_FAILED_SOFT,
                              tr("This Microsoft account requires proof of age to play. Please login to %1 to provide proof of age.")
                                  .arg("<a href=\"https://login.live.com/login.srf\">login.live.com</a>"));
                return true;
            }
            case 2148916237:
                emit finished(AccountTaskState::STATE_FAILED_SOFT, tr("This Microsoft account has reached its limit for playtime. This "
                                                                      "Microsoft account has been blocked from logging in."));
                return true;
            case 2148916227: {
                emit finished(AccountTaskState::STATE_FAILED_SOFT, tr("This Microsoft account was banned by Xbox for violating one or more "
                                                                      "Community Standards for Xbox and is unable to be used."));
                return true;
            }
            case 2148916229: {
                emit finished(AccountTaskState::STATE_FAILED_SOFT,
                              tr("This Microsoft account is currently restricted and your guardian has not given you permission to play "
                                 "online. Login to %1 and have your guardian change your permissions.")
                                  .arg("<a href=\"https://account.microsoft.com/family/\">account.microsoft.com</a>"));
                return true;
            }
            case 2148916234: {
                emit finished(AccountTaskState::STATE_FAILED_SOFT,
                              tr("This Microsoft account has not accepted Xbox's Terms of Service. Please login and accept them."));
                return true;
            }
            default: {
                emit finished(AccountTaskState::STATE_FAILED_SOFT,
                              tr("XSTS authentication ended with unrecognized error(s):\n\n%1").arg(errorCode));
                return true;
            }
        }
    }
    return false;
}
