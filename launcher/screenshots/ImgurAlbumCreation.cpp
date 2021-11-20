#include "ImgurAlbumCreation.h"

#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QStringList>

#include "BuildConfig.h"
#include "Env.h"
#include <QDebug>

ImgurAlbumCreation::ImgurAlbumCreation(QList<ScreenshotPtr> screenshots) : NetAction(), m_screenshots(screenshots)
{
    m_url = BuildConfig.IMGUR_BASE_URL + "album.json";
    m_status = Job_NotStarted;
}

void ImgurAlbumCreation::start()
{
    m_status = Job_InProgress;
    QNetworkRequest request(m_url);
    request.setHeader(QNetworkRequest::UserAgentHeader, BuildConfig.USER_AGENT_UNCACHED);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("Authorization", QString("Client-ID %1").arg(BuildConfig.IMGUR_CLIENT_ID).toStdString().c_str());
    request.setRawHeader("Accept", "application/json");

    QStringList hashes;
    for (auto shot : m_screenshots)
    {
        hashes.append(shot->m_imgurDeleteHash);
    }

    const QByteArray data = "deletehashes=" + hashes.join(',').toUtf8() + "&title=Minecraft%20Screenshots&privacy=hidden";

    QNetworkReply *rep = ENV->network().post(request, data);

    m_reply.reset(rep);
    connect(rep, &QNetworkReply::uploadProgress, this, &ImgurAlbumCreation::downloadProgress);
    connect(rep, &QNetworkReply::finished, this, &ImgurAlbumCreation::downloadFinished);
    connect(rep, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(downloadError(QNetworkReply::NetworkError)));
}
void ImgurAlbumCreation::downloadError(QNetworkReply::NetworkError error)
{
    qDebug() << m_reply->errorString();
    m_status = Job_Failed;
}
void ImgurAlbumCreation::downloadFinished()
{
    if (m_status != Job_Failed)
    {
        QByteArray data = m_reply->readAll();
        m_reply.reset();
        QJsonParseError jsonError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
        if (jsonError.error != QJsonParseError::NoError)
        {
            qDebug() << jsonError.errorString();
            emit failed(m_index_within_job);
            return;
        }
        auto object = doc.object();
        if (!object.value("success").toBool())
        {
            qDebug() << doc.toJson();
            emit failed(m_index_within_job);
            return;
        }
        m_deleteHash = object.value("data").toObject().value("deletehash").toString();
        m_id = object.value("data").toObject().value("id").toString();
        m_status = Job_Finished;
        emit succeeded(m_index_within_job);
        return;
    }
    else
    {
        qDebug() << m_reply->readAll();
        m_reply.reset();
        emit failed(m_index_within_job);
        return;
    }
}
void ImgurAlbumCreation::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    m_total_progress = bytesTotal;
    m_progress = bytesReceived;
    emit netActionProgress(m_index_within_job, bytesReceived, bytesTotal);
}
