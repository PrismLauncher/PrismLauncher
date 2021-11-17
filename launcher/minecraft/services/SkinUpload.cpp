#include "SkinUpload.h"
#include <QNetworkRequest>
#include <QHttpMultiPart>
#include <Env.h>

QByteArray getVariant(SkinUpload::Model model) {
    switch (model) {
        default:
            qDebug() << "Unknown skin type!";
        case SkinUpload::STEVE:
            return "CLASSIC";
        case SkinUpload::ALEX:
            return "SLIM";
    }
}

SkinUpload::SkinUpload(QObject *parent, AuthSessionPtr session, QByteArray skin, SkinUpload::Model model)
    : Task(parent), m_model(model), m_skin(skin), m_session(session)
{
}

void SkinUpload::executeTask()
{
    QNetworkRequest request(QUrl("https://api.minecraftservices.com/minecraft/profile/skins"));
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_session->access_token).toLocal8Bit());
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart skin;
    skin.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/png"));
    skin.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"file\"; filename=\"skin.png\""));
    skin.setBody(m_skin);

    QHttpPart model;
    model.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"variant\""));
    model.setBody(getVariant(m_model));

    multiPart->append(skin);
    multiPart->append(model);

    QNetworkReply *rep = ENV.network().post(request, multiPart);
    m_reply = std::shared_ptr<QNetworkReply>(rep);

    setStatus(tr("Uploading skin"));
    connect(rep, &QNetworkReply::uploadProgress, this, &Task::setProgress);
    connect(rep, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(downloadError(QNetworkReply::NetworkError)));
    connect(rep, SIGNAL(finished()), this, SLOT(downloadFinished()));
}

void SkinUpload::downloadError(QNetworkReply::NetworkError error)
{
    // error happened during download.
    qCritical() << "Network error: " << error;
    emitFailed(m_reply->errorString());
}

void SkinUpload::downloadFinished()
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
