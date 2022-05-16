#include "CapeChange.h"

#include <QNetworkRequest>
#include <QHttpMultiPart>

#include "Application.h"

CapeChange::CapeChange(QObject *parent, QString token, QString cape)
    : Task(parent), m_capeId(cape), m_token(token)
{
}

void CapeChange::setCape(QString& cape) {
    QNetworkRequest request(QUrl("https://api.minecraftservices.com/minecraft/profile/capes/active"));
    auto requestString = QString("{\"capeId\":\"%1\"}").arg(m_capeId);
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_token).toLocal8Bit());
    QNetworkReply *rep = APPLICATION->network()->put(request, requestString.toUtf8());

    setStatus(tr("Equipping cape"));

    m_reply = shared_qobject_ptr<QNetworkReply>(rep);
    connect(rep, &QNetworkReply::uploadProgress, this, &Task::setProgress);
    connect(rep, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(downloadError(QNetworkReply::NetworkError)));
    connect(rep, SIGNAL(finished()), this, SLOT(downloadFinished()));
}

void CapeChange::clearCape() {
    QNetworkRequest request(QUrl("https://api.minecraftservices.com/minecraft/profile/capes/active"));
    auto requestString = QString("{\"capeId\":\"%1\"}").arg(m_capeId);
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_token).toLocal8Bit());
    QNetworkReply *rep = APPLICATION->network()->deleteResource(request);

    setStatus(tr("Removing cape"));

    m_reply = shared_qobject_ptr<QNetworkReply>(rep);
    connect(rep, &QNetworkReply::uploadProgress, this, &Task::setProgress);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    connect(rep, SIGNAL(errorOccurred(QNetworkReply::NetworkError)), this, SLOT(downloadError(QNetworkReply::NetworkError)));
#else
    connect(rep, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(downloadError(QNetworkReply::NetworkError)));
#endif
    connect(rep, SIGNAL(finished()), this, SLOT(downloadFinished()));
}


void CapeChange::executeTask()
{
    if(m_capeId.isEmpty()) {
        clearCape();
    }
    else {
        setCape(m_capeId);
    }
}

void CapeChange::downloadError(QNetworkReply::NetworkError error)
{
    // error happened during download.
    qCritical() << "Network error: " << error;
    emitFailed(m_reply->errorString());
}

void CapeChange::downloadFinished()
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
