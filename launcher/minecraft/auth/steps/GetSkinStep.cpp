
#include "GetSkinStep.h"

#include <QNetworkRequest>

#include "Application.h"

GetSkinStep::GetSkinStep(AccountData* data) : AuthStep(data) {}

QString GetSkinStep::describe()
{
    return tr("Getting skin.");
}

void GetSkinStep::perform()
{
    QUrl url(m_data->minecraftProfile.skin.url);

    m_response.reset(new QByteArray());
    m_task = Net::Download::makeByteArray(url, m_response);

    connect(m_task.get(), &Task::finished, this, &GetSkinStep::onRequestDone);

    m_task->setNetwork(APPLICATION->network());
    m_task->start();
}

void GetSkinStep::onRequestDone()
{
    if (m_task->error() == QNetworkReply::NoError)
        m_data->minecraftProfile.skin.data = *m_response;
    emit finished(AccountTaskState::STATE_SUCCEEDED, tr("Got skin"));
}
