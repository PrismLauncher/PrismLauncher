#include "ImgurUpload.h"
#include "BuildConfig.h"

#include <QNetworkRequest>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHttpPart>
#include <QFile>
#include <QUrl>
#include <QDebug>

ImgurUpload::ImgurUpload(ScreenShot::Ptr shot) : NetAction(), m_shot(shot)
{
    m_url = BuildConfig.IMGUR_BASE_URL + "upload.json";
    m_state = State::Inactive;
}

void ImgurUpload::executeTask()
{
    finished = false;
    m_state = Task::State::Running;
    QNetworkRequest request(m_url);
    request.setHeader(QNetworkRequest::UserAgentHeader, BuildConfig.USER_AGENT_UNCACHED);
    request.setRawHeader("Authorization", QString("Client-ID %1").arg(BuildConfig.IMGUR_CLIENT_ID).toStdString().c_str());
    request.setRawHeader("Accept", "application/json");

    QFile f(m_shot->m_file.absoluteFilePath());
    if (!f.open(QFile::ReadOnly))
    {
        emitFailed();
        return;
    }

    QHttpMultiPart *multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart filePart;
    filePart.setBody(f.readAll().toBase64());
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, "image/png");
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"image\"");
    multipart->append(filePart);
    QHttpPart typePart;
    typePart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"type\"");
    typePart.setBody("base64");
    multipart->append(typePart);
    QHttpPart namePart;
    namePart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"name\"");
    namePart.setBody(m_shot->m_file.baseName().toUtf8());
    multipart->append(namePart);

    QNetworkReply *rep = m_network->post(request, multipart);

    m_reply.reset(rep);
    connect(rep, &QNetworkReply::uploadProgress, this, &ImgurUpload::downloadProgress);
    connect(rep, &QNetworkReply::finished, this, &ImgurUpload::downloadFinished);
    connect(rep, SIGNAL(error(QNetworkReply::NetworkError)),
            SLOT(downloadError(QNetworkReply::NetworkError)));
}
void ImgurUpload::downloadError(QNetworkReply::NetworkError error)
{
    qCritical() << "ImgurUpload failed with error" << m_reply->errorString() << "Server reply:\n" << m_reply->readAll();
    if(finished)
    {
        qCritical() << "Double finished ImgurUpload!";
        return;
    }
    m_state = Task::State::Failed;
    finished = true;
    m_reply.reset();
    emitFailed();
}
void ImgurUpload::downloadFinished()
{
    if(finished)
    {
        qCritical() << "Double finished ImgurUpload!";
        return;
    }
    QByteArray data = m_reply->readAll();
    m_reply.reset();
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
    if (jsonError.error != QJsonParseError::NoError)
    {
        qDebug() << "imgur server did not reply with JSON" << jsonError.errorString();
        finished = true;
        m_reply.reset();
        emitFailed();
        return;
    }
    auto object = doc.object();
    if (!object.value("success").toBool())
    {
        qDebug() << "Screenshot upload not successful:" << doc.toJson();
        finished = true;
        m_reply.reset();
        emitFailed();
        return;
    }
    m_shot->m_imgurId = object.value("data").toObject().value("id").toString();
    m_shot->m_url = object.value("data").toObject().value("link").toString();
    m_shot->m_imgurDeleteHash = object.value("data").toObject().value("deletehash").toString();
    m_state = Task::State::Succeeded;
    finished = true;
    emit succeeded();
    return;
}
void ImgurUpload::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    setProgress(bytesReceived, bytesTotal);
    emit progress(bytesReceived, bytesTotal);
}
