#include "EntitlementsStep.h"

#include <QList>
#include <QNetworkRequest>
#include <QUrl>
#include <QUuid>
#include <memory>

#include "Application.h"
#include "Logging.h"
#include "minecraft/auth/Parsers.h"
#include "net/Download.h"
#include "net/StaticHeaderProxy.h"
#include "tasks/Task.h"

EntitlementsStep::EntitlementsStep(AccountData* data) : AuthStep(data) {}

QString EntitlementsStep::describe()
{
    return tr("Determining game ownership.");
}

void EntitlementsStep::perform()
{
    auto uuid = QUuid::createUuid();
    m_entitlements_request_id = uuid.toString().remove('{').remove('}');

    QUrl url("https://api.minecraftservices.com/entitlements/license?requestId=" + m_entitlements_request_id);
    auto headers = QList<Net::HeaderPair>{ { "Content-Type", "application/json" },
                                           { "Accept", "application/json" },
                                           { "Authorization", QString("Bearer %1").arg(m_data->yggdrasilToken.token).toUtf8() } };

    m_response.reset(new QByteArray());
    m_task = Net::Download::makeByteArray(url, m_response);
    m_task->addHeaderProxy(new Net::StaticHeaderProxy(headers));

    connect(m_task.get(), &Task::finished, this, &EntitlementsStep::onRequestDone);

    m_task->setNetwork(APPLICATION->network());
    m_task->start();
    qDebug() << "Getting entitlements...";
}

void EntitlementsStep::onRequestDone()
{
    qCDebug(authCredentials()) << *m_response;

    // TODO: check presence of same entitlementsRequestId?
    // TODO: validate JWTs?
    Parsers::parseMinecraftEntitlements(*m_response, m_data->minecraftEntitlement);

    emit finished(AccountTaskState::STATE_WORKING, tr("Got entitlements"));
}
