
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
    m_request = Net::Download::makeByteArray(url, m_response);

    m_task.reset(new NetJob("GetSkinStep", APPLICATION->network()));
    m_task->setAskRetry(false);
    m_task->addNetAction(m_request);

    connect(m_task.get(), &Task::finished, this, &GetSkinStep::onRequestDone);

    m_task->start();
}

void GetSkinStep::onRequestDone()
{
    if (m_request->error() == QNetworkReply::NoError)
        m_data->minecraftProfile.skin.data = *m_response;
    emit finished(AccountTaskState::STATE_WORKING, tr("Got skin"));
}
