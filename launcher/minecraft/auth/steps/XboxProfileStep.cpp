#include "XboxProfileStep.h"

#include <QNetworkRequest>
#include <QUrlQuery>


#include "minecraft/auth/AuthRequest.h"
#include "minecraft/auth/Parsers.h"
#include "net/NetUtils.h"

XboxProfileStep::XboxProfileStep(AccountData* data) : AuthStep(data) {

}

XboxProfileStep::~XboxProfileStep() noexcept = default;

QString XboxProfileStep::describe() {
    return tr("Fetching Xbox profile.");
}

void XboxProfileStep::rehydrate() {
    // NOOP, for now. We only save bools and there's nothing to check.
}

void XboxProfileStep::perform() {
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
    connect(requestor, &AuthRequest::finished, this, &XboxProfileStep::onRequestDone);
    requestor->get(request);
    qCDebug(LAUNCHER_LOG) << "Getting Xbox profile...";
}

void XboxProfileStep::onRequestDone(
    QNetworkReply::NetworkError error,
    QByteArray data,
    QList<QNetworkReply::RawHeaderPair> headers
) {
    auto requestor = qobject_cast<AuthRequest *>(QObject::sender());
    requestor->deleteLater();

    if (error != QNetworkReply::NoError) {
        qCWarning(LAUNCHER_LOG) << "Reply error:" << error;
#ifndef NDEBUG
        qCDebug(LAUNCHER_LOG) << data;
#endif
        if (Net::isApplicationError(error)) {
            emit finished(
                AccountTaskState::STATE_FAILED_SOFT,
                tr("Failed to retrieve the Xbox profile: %1").arg(requestor->errorString_)
            );
        }
        else {
            emit finished(
                AccountTaskState::STATE_OFFLINE,
                tr("Failed to retrieve the Xbox profile: %1").arg(requestor->errorString_)
            );
        }
        return;
    }

#ifndef NDEBUG
    qCDebug(LAUNCHER_LOG) << "XBox profile: " << data;
#endif

    emit finished(AccountTaskState::STATE_WORKING, tr("Got Xbox profile"));
}
