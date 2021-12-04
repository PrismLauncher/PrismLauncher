
#include "GetSkinStep.h"

#include <QNetworkRequest>

#include "minecraft/auth/AuthRequest.h"
#include "minecraft/auth/Parsers.h"

GetSkinStep::GetSkinStep(AccountData* data) : AuthStep(data) {

}

GetSkinStep::~GetSkinStep() noexcept = default;

QString GetSkinStep::describe() {
    return tr("Getting skin.");
}

void GetSkinStep::perform() {
    auto url = QUrl(m_data->minecraftProfile.skin.url);
    QNetworkRequest request = QNetworkRequest(url);
    AuthRequest *requestor = new AuthRequest(this);
    connect(requestor, &AuthRequest::finished, this, &GetSkinStep::onRequestDone);
    requestor->get(request);
}

void GetSkinStep::rehydrate() {
    // NOOP, for now.
}

void GetSkinStep::onRequestDone(
    QNetworkReply::NetworkError error,
    QByteArray data,
    QList<QNetworkReply::RawHeaderPair> headers
) {
    auto requestor = qobject_cast<AuthRequest *>(QObject::sender());
    requestor->deleteLater();

    if (error == QNetworkReply::NoError) {
        m_data->minecraftProfile.skin.data = data;
    }
    emit finished(AccountTaskState::STATE_SUCCEEDED, tr("Got skin"));
}
