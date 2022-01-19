#include "PasteUpload.h"
#include "BuildConfig.h"
#include "Application.h"

#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>

PasteUpload::PasteUpload(QWidget *window, QString text, QString url) : m_window(window), m_uploadUrl(url), m_text(text.toUtf8())
{
}

PasteUpload::~PasteUpload()
{
}

void PasteUpload::executeTask()
{
    QNetworkRequest request{QUrl(m_uploadUrl)};
    request.setHeader(QNetworkRequest::UserAgentHeader, BuildConfig.USER_AGENT_UNCACHED);

    QHttpMultiPart *multiPart = new QHttpMultiPart{QHttpMultiPart::FormDataType};

    QHttpPart filePart;
    filePart.setBody(m_text);
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"file\"; filename=\"log.txt\"");

    multiPart->append(filePart);

    QNetworkReply *rep = APPLICATION->network()->post(request, multiPart);
    multiPart->setParent(rep);

    m_reply = std::shared_ptr<QNetworkReply>(rep);
    setStatus(tr("Uploading to %1").arg(m_uploadUrl));

    connect(rep, &QNetworkReply::uploadProgress, this, &Task::setProgress);
    connect(rep, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(downloadError(QNetworkReply::NetworkError)));
    connect(rep, SIGNAL(finished()), this, SLOT(downloadFinished()));
}

void PasteUpload::downloadError(QNetworkReply::NetworkError error)
{
    // error happened during download.
    qCritical() << "Network error: " << error;
    emitFailed(m_reply->errorString());
}

void PasteUpload::downloadFinished()
{
    QByteArray data = m_reply->readAll();
    int statusCode = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (m_reply->error() != QNetworkReply::NetworkError::NoError)
    {
        emitFailed(tr("Network error: %1").arg(m_reply->errorString()));
        m_reply.reset();
        return;
    }
    else if (statusCode != 200)
    {
        QString reasonPhrase = m_reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
        emitFailed(tr("Error: %1 returned unexpected status code %2 %3").arg(m_uploadUrl).arg(statusCode).arg(reasonPhrase));
        qCritical() << m_uploadUrl << " returned unexpected status code " << statusCode << " with body: " << data;
        m_reply.reset();
        return;
    }

    m_pasteLink = QString::fromUtf8(data).trimmed();
    emitSucceeded();
}
