#include "SkinDelete.h"

#include <QNetworkRequest>
#include <QHttpMultiPart>

#include "Application.h"

SkinDelete::SkinDelete(QObject *parent, QString token)
    : Task(parent), m_token(token)
{
}

void SkinDelete::executeTask()
{
    QNetworkRequest request(QUrl("https://api.minecraftservices.com/minecraft/profile/skins/active"));
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_token).toLocal8Bit());
    QNetworkReply *rep = APPLICATION->network()->deleteResource(request);
    m_reply = shared_qobject_ptr<QNetworkReply>(rep);

    setStatus(tr("Deleting skin"));
    connect(rep, &QNetworkReply::uploadProgress, this, &Task::setProgress);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    connect(rep, SIGNAL(errorOccurred(QNetworkReply::NetworkError)), this, SLOT(downloadError(QNetworkReply::NetworkError)));
#else
    connect(rep, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(downloadError(QNetworkReply::NetworkError)));
#endif
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

