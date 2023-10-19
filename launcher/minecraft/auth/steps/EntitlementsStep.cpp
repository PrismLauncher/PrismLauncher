#include "EntitlementsStep.h"

#include <QNetworkRequest>
#include <QUuid>

#include "Logging.h"
#include "minecraft/auth/AuthRequest.h"
#include "minecraft/auth/Parsers.h"

EntitlementsStep::EntitlementsStep(AccountData* data) : AuthStep(data) {}

EntitlementsStep::~EntitlementsStep() noexcept = default;

QString EntitlementsStep::describe()
{
    return tr("Determining game ownership.");
}

void EntitlementsStep::perform()
{
    auto uuid = QUuid::createUuid();
    m_entitlementsRequestId = uuid.toString().remove('{').remove('}');
    auto url = "https://api.minecraftservices.com/entitlements/license?requestId=" + m_entitlementsRequestId;
    QNetworkRequest request = QNetworkRequest(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_data->yggdrasilToken.token).toUtf8());
    AuthRequest* requestor = new AuthRequest(this);
    connect(requestor, &AuthRequest::finished, this, &EntitlementsStep::onRequestDone);
    requestor->get(request);
    qDebug() << "Getting entitlements...";
}

void EntitlementsStep::rehydrate()
{
    // NOOP, for now. We only save bools and there's nothing to check.
}

void EntitlementsStep::onRequestDone([[maybe_unused]] QNetworkReply::NetworkError error,
                                     QByteArray data,
                                     [[maybe_unused]] QList<QNetworkReply::RawHeaderPair> headers)
{
    auto requestor = qobject_cast<AuthRequest*>(QObject::sender());
    requestor->deleteLater();

    qCDebug(authCredentials()) << data;

    // TODO: check presence of same entitlementsRequestId?
    // TODO: validate JWTs?
    Parsers::parseMinecraftEntitlements(data, m_data->minecraftEntitlement);

    emit finished(AccountTaskState::STATE_WORKING, tr("Got entitlements"));
}
