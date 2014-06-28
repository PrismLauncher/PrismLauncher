#include "ImgurAlbumCreation.h"

#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

#include "logic/net/URLConstants.h"
#include "MultiMC.h"
#include "logger/QsLog.h"

ImgurAlbumCreation::ImgurAlbumCreation(QList<ScreenshotPtr> screenshots) : NetAction(), m_screenshots(screenshots)
{
	m_url = URLConstants::IMGUR_BASE_URL + "album.json";
	m_status = Job_NotStarted;
}

void ImgurAlbumCreation::start()
{
	m_status = Job_InProgress;
	QNetworkRequest request(m_url);
	request.setHeader(QNetworkRequest::UserAgentHeader, "MultiMC/5.0 (Uncached)");
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
	request.setRawHeader("Authorization", "Client-ID 5b97b0713fba4a3");
	request.setRawHeader("Accept", "application/json");

	QStringList ids;
	for (auto shot : m_screenshots)
	{
		ids.append(shot->m_imgurId);
	}

	const QByteArray data = "ids=" + ids.join(',').toUtf8() + "&title=Minecraft%20Screenshots&privacy=hidden";

	auto worker = MMC->qnam();
	QNetworkReply *rep = worker->post(request, data);

	m_reply = std::shared_ptr<QNetworkReply>(rep);
	connect(rep, &QNetworkReply::uploadProgress, this, &ImgurAlbumCreation::downloadProgress);
	connect(rep, &QNetworkReply::finished, this, &ImgurAlbumCreation::downloadFinished);
	connect(rep, SIGNAL(error(QNetworkReply::NetworkError)),
			SLOT(downloadError(QNetworkReply::NetworkError)));
}
void ImgurAlbumCreation::downloadError(QNetworkReply::NetworkError error)
{
	QLOG_DEBUG() << m_reply->errorString();
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
		m_deleteHash = object.value("data").toObject().value("deletehash").toString();
		m_id = object.value("data").toObject().value("id").toString();
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
void ImgurAlbumCreation::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	m_total_progress = bytesTotal;
	m_progress = bytesReceived;
	emit progress(m_index_within_job, bytesReceived, bytesTotal);
}
