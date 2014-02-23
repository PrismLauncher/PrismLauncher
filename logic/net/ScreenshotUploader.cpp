#include "ScreenshotUploader.h"
#include "logic/lists/ScreenshotList.h"
#include <QNetworkRequest>
#include <QJsonObject>
#include <QJsonDocument>
#include "URLConstants.h"
#include "MultiMC.h"

ScreenShotUpload::ScreenShotUpload(ScreenShot *shot) : m_shot(shot)
{
	m_status = Job_NotStarted;
}

void ScreenShotUpload::start()
{
	m_status = Job_InProgress;
	QNetworkRequest request(URLConstants::IMGUR_UPLOAD_URL);
	request.setHeader(QNetworkRequest::UserAgentHeader, "MultiMC/5.0 (Uncached)");
	request.setRawHeader("Authorization", "Client-ID 5b97b0713fba4a3");

	QJsonObject object;
	object.insert("image", QJsonValue::fromVariant(m_shot->file.toBase64()));
	object.insert("type", QJsonValue::fromVariant("base64"));
	object.insert("name", QJsonValue::fromVariant(m_shot->timestamp));
	QJsonDocument doc;
	doc.setObject(object);

	auto worker = MMC->qnam();
	QNetworkReply *rep = worker->post(request, doc.toJson());

	m_reply = std::shared_ptr<QNetworkReply>(rep);
	connect(rep, SIGNAL(downloadProgress(qint64, qint64)),
			SLOT(downloadProgress(qint64, qint64)));
	connect(rep, SIGNAL(finished()), SLOT(downloadFinished()));
	connect(rep, SIGNAL(error(QNetworkReply::NetworkError)),
			SLOT(downloadError(QNetworkReply::NetworkError)));
	connect(rep, SIGNAL(readyRead()), SLOT(downloadReadyRead()));
}
void ScreenShotUpload::downloadError(QNetworkReply::NetworkError error)
{
	m_status = Job_Failed;
}
void ScreenShotUpload::downloadFinished()
{
	if (m_status != Job_Failed)
	{
		QByteArray data = m_reply->readAll();
		m_reply.reset();
		QJsonParseError jsonError;
		QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
		if (jsonError.error != QJsonParseError::NoError)
		{
			emit failed(m_index_within_job);
			return;
		}
		auto object = doc.object();
		if (!object.value("success").toBool())
		{
			emit failed(m_index_within_job);
			return;
		}
		m_shot->imgurIndex = object.value("data").toVariant().toInt();
		m_status = Job_Finished;
		emit succeeded(m_index_within_job);
		return;
	}
	else
	{
		m_reply.reset();
		emit failed(m_index_within_job);
		return;
	}
}
void ScreenShotUpload::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	m_total_progress = bytesTotal;
	m_progress = bytesReceived;
	emit progress(m_index_within_job, bytesReceived, bytesTotal);
}
void ScreenShotUpload::downloadReadyRead()
{
	// noop
}
ScreenShotGet::ScreenShotGet(ScreenShot *shot) : m_shot(shot)
{
	m_status = Job_NotStarted;
}
void ScreenShotGet::start()
{
	m_status = Job_InProgress;
	QNetworkRequest request(URLConstants::IMGUR_GET_BASE + m_shot->imgurIndex + ".json");
	request.setHeader(QNetworkRequest::UserAgentHeader, "MultiMC/5.0 (Uncached)");
	request.setRawHeader("Authorization", "Client-ID 5b97b0713fba4a3");

	auto worker = MMC->qnam();
	QNetworkReply *rep = worker->get(request);

	m_reply = std::shared_ptr<QNetworkReply>(rep);
	connect(rep, SIGNAL(downloadProgress(qint64, qint64)),
			SLOT(downloadProgress(qint64, qint64)));
	connect(rep, SIGNAL(finished()), SLOT(downloadFinished()));
	connect(rep, SIGNAL(error(QNetworkReply::NetworkError)),
			SLOT(downloadError(QNetworkReply::NetworkError)));
	connect(rep, SIGNAL(readyRead()), SLOT(downloadReadyRead()));
}
void ScreenShotGet::downloadError(QNetworkReply::NetworkError error)
{
	m_status = Job_Failed;
}
void ScreenShotGet::downloadFinished()
{
}
void ScreenShotGet::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	m_total_progress = bytesTotal;
	m_progress = bytesReceived;
	emit progress(m_index_within_job, bytesReceived, bytesTotal);
}
void ScreenShotGet::downloadReadyRead()
{
	if (m_status != Job_Failed)
	{
		QByteArray data = m_reply->readAll();
		m_reply.reset();
		QJsonParseError jsonError;
		QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
		if (jsonError.error != QJsonParseError::NoError)
		{
			emit failed(m_index_within_job);
			return;
		}
		auto object = doc.object();
		m_shot->url = object.value("link").toString();
		m_status = Job_Finished;
		emit succeeded(m_index_within_job);
		return;
	}
	else
	{
		m_reply.reset();
		emit failed(m_index_within_job);
		return;
	}
}
