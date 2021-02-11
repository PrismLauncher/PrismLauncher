#include "SkinDelete.h"
#include <QNetworkRequest>
#include <QHttpMultiPart>
#include <Env.h>

SkinDelete::SkinDelete(QObject *parent, AuthSessionPtr session)
    : Task(parent), m_session(session)
{
}

void SkinDelete::executeTask()
{
    QNetworkRequest request(QUrl("https://api.minecraftservices.com/minecraft/profile/skins/active"));
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_session->access_token).toLocal8Bit());
    QNetworkReply *rep = ENV.qnam().deleteResource(request);
    m_reply = std::shared_ptr<QNetworkReply>(rep);

    setStatus(tr("Deleting skin"));
    connect(rep, &QNetworkReply::uploadProgress, this, &Task::setProgress);
    connect(rep, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(downloadError(QNetworkReply::NetworkError)));
    connect(rep, SIGNAL(finished()), this, SLOT(downloadFinished()));
}

void SkinDelete::downloadError(QNetworkReply::NetworkError error)
{
    // error happened during download.
    qCritical() << "Network error: " << error;
    emitFailed(m_reply->errorString());
}

void SkinDelete::downloadFinished()
{
    // if the download failed
    if (m_reply->error() != QNetworkReply::NetworkError::NoError)
    {
        emitFailed(QString("Network error: %1").arg(m_reply->errorString()));
        m_reply.reset();
        return;
    }
    emitSucceeded();
}

