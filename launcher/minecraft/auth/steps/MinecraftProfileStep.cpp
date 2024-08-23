#include "MinecraftProfileStep.h"

#include <QNetworkRequest>

#include "Application.h"
#include "minecraft/auth/Parsers.h"
#include "net/NetUtils.h"
#include "net/RawHeaderProxy.h"

MinecraftProfileStep::MinecraftProfileStep(AccountData* data) : AuthStep(data) {}

QString MinecraftProfileStep::describe()
{
    return tr("Fetching the Minecraft profile.");
}

void MinecraftProfileStep::perform()
{
    QUrl url("https://api.minecraftservices.com/minecraft/profile");
    auto headers = QList<Net::HeaderPair>{ { "Content-Type", "application/json" },
                                           { "Accept", "application/json" },
                                           { "Authorization", QString("Bearer %1").arg(m_data->yggdrasilToken.token).toUtf8() } };

    m_response.reset(new QByteArray());
    m_request = Net::Download::makeByteArray(url, m_response);
    m_request->addHeaderProxy(new Net::RawHeaderProxy(headers));

    m_task.reset(new NetJob("MinecraftProfileStep", APPLICATION->network()));
    m_task->setAskRetry(false);
    m_task->addNetAction(m_request);

    connect(m_task.get(), &Task::finished, this, &MinecraftProfileStep::onRequestDone);

    m_task->start();
}

void MinecraftProfileStep::onRequestDone()
{
    if (m_request->error() == QNetworkReply::ContentNotFoundError) {
        // NOTE: Succeed even if we do not have a profile. This is a valid account state.
        m_data->minecraftProfile = MinecraftProfile();
        emit finished(AccountTaskState::STATE_WORKING, tr("Account has no Minecraft profile."));
        return;
    }
    if (m_request->error() != QNetworkReply::NoError) {
        qWarning() << "Error getting profile:";
        qWarning() << " HTTP Status:        " << m_request->replyStatusCode();
        qWarning() << " Internal error no.: " << m_request->error();
        qWarning() << " Error string:       " << m_request->errorString();

        qWarning() << " Response:";
        qWarning() << QString::fromUtf8(*m_response);

        if (Net::isApplicationError(m_request->error())) {
            emit finished(AccountTaskState::STATE_FAILED_SOFT,
                          tr("Minecraft Java profile acquisition failed: %1").arg(m_request->errorString()));
        } else {
            emit finished(AccountTaskState::STATE_OFFLINE,
                          tr("Minecraft Java profile acquisition failed: %1").arg(m_request->errorString()));
        }
        return;
    }
    if (!Parsers::parseMinecraftProfile(*m_response, m_data->minecraftProfile)) {
        m_data->minecraftProfile = MinecraftProfile();
        emit finished(AccountTaskState::STATE_FAILED_SOFT, tr("Minecraft Java profile response could not be parsed"));
        return;
    }

    emit finished(AccountTaskState::STATE_WORKING, tr("Minecraft Java profile acquisition succeeded."));
}
