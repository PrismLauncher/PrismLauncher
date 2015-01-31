#include "ImgurUpload.h"

#include <QNetworkRequest>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHttpPart>
#include <QFile>
#include <QUrl>

#include "logic/net/URLConstants.h"
#include "logic/Env.h"
#include "logger/QsLog.h"

ImgurUpload::ImgurUpload(ScreenshotPtr shot) : NetAction(), m_shot(shot)
{
	m_url = URLConstants::IMGUR_BASE_URL + "upload.json";
	m_status = Job_NotStarted;
}

void ImgurUpload::start()
{
	m_status = Job_InProgress;
	QNetworkRequest request(m_url);
	request.setHeader(QNetworkRequest::UserAgentHeader, "MultiMC/5.0 (Uncached)");
	request.setRawHeader("Authorization", "Client-ID 5b97b0713fba4a3");
	request.setRawHeader("Accept", "application/json");

	QFile f(m_shot->m_file.absoluteFilePath());
	if (!f.open(QFile::ReadOnly))
	{
		emit failed(m_index_within_job);
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

	auto worker = ENV.qnam();
	QNetworkReply *rep = worker->post(request, multipart);

	m_reply = std::shared_ptr<QNetworkReply>(rep);
	connect(rep, &QNetworkReply::uploadProgress, this, &ImgurUpload::downloadProgress);
	connect(rep, &QNetworkReply::finished, this, &ImgurUpload::downloadFinished);
	connect(rep, SIGNAL(error(QNetworkReply::NetworkError)),
			SLOT(downloadError(QNetworkReply::NetworkError)));
}
void ImgurUpload::downloadError(QNetworkReply::NetworkError error)
{
	QLOG_DEBUG() << m_reply->errorString();
	m_status = Job_Failed;
}
void ImgurUpload::downloadFinished()
{
	if (m_status != Job_Failed)
	{
		QByteArray data = m_reply->readAll();
		m_reply.reset();
		QJsonParseError jsonError;
		QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
		if (jsonError.error != QJsonParseError::NoError)
		{
			QLOG_DEBUG() << jsonError.errorString();
			emit failed(m_index_within_job);
			return;
		}
		auto object = doc.object();
		if (!object.value("success").toBool())
		{
			QLOG_DEBUG() << doc.toJson();
			emit failed(m_index_within_job);
			return;
		}
		m_shot->m_imgurId = object.value("data").toObject().value("id").toString();
		m_shot->m_url = object.value("data").toObject().value("link").toString();
		m_status = Job_Finished;
		emit succeeded(m_index_within_job);
		return;
	}
	else
	{
		QLOG_DEBUG() << m_reply->readAll();
		m_reply.reset();
		emit failed(m_index_within_job);
		return;
	}
}
void ImgurUpload::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	m_total_progress = bytesTotal;
	m_progress = bytesReceived;
	emit progress(m_index_within_job, bytesReceived, bytesTotal);
}
