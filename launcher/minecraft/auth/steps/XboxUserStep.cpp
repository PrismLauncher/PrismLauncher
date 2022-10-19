#include "XboxUserStep.h"

#include <QNetworkRequest>

#include "minecraft/auth/AuthRequest.h"
#include "minecraft/auth/Parsers.h"
#include "net/NetUtils.h"

XboxUserStep::XboxUserStep(AccountData* data) : AuthStep(data) {

}

XboxUserStep::~XboxUserStep() noexcept = default;

QString XboxUserStep::describe() {
    return tr("Logging in as an Xbox user.");
}


void XboxUserStep::rehydrate() {
    // NOOP, for now. We only save bools and there's nothing to check.
}

void XboxUserStep::perform() {
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
    connect(requestor, &AuthRequest::finished, this, &XboxUserStep::onRequestDone);
    requestor->post(request, xbox_auth_data.toUtf8());
    qCDebug(LAUNCHER_LOG) << "First layer of XBox auth ... commencing.";
}

void XboxUserStep::onRequestDone(
    QNetworkReply::NetworkError error,
    QByteArray data,
    QList<QNetworkReply::RawHeaderPair> headers
) {
    auto requestor = qobject_cast<AuthRequest *>(QObject::sender());
    requestor->deleteLater();

    if (error != QNetworkReply::NoError) {
        qCWarning(LAUNCHER_LOG) << "Reply error:" << error;
        if (Net::isApplicationError(error)) {
            emit finished(AccountTaskState::STATE_FAILED_SOFT,
                tr("XBox user authentication failed: %1").arg(requestor->errorString_)
            );
        }
        else {
            emit finished(
                AccountTaskState::STATE_OFFLINE,
                tr("XBox user authentication failed: %1").arg(requestor->errorString_)
            );
        }
        return;
    }

    Katabasis::Token temp;
    if(!Parsers::parseXTokenResponse(data, temp, "UToken")) {
        qCWarning(LAUNCHER_LOG) << "Could not parse user authentication response...";
        emit finished(AccountTaskState::STATE_FAILED_SOFT, tr("XBox user authentication response could not be understood."));
        return;
    }
    m_data->userToken = temp;
    emit finished(AccountTaskState::STATE_WORKING, tr("Got Xbox user token"));
}
