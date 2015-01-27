#include "PasteUpload.h"
#include "MultiMC.h"
#include "logger/QsLog.h"
#include <QJsonObject>
#include <QJsonDocument>

PasteUpload::PasteUpload(QWidget *window, QString text) : m_window(window)
{
	m_text = text.toUtf8();
	m_text.replace('\n', "\r\n");
}

bool PasteUpload::validateText()
{
	return m_text.size() <= maxSize();
}

void PasteUpload::executeTask()
{
	QNetworkRequest request(QUrl("http://paste.ee/api"));
	request.setHeader(QNetworkRequest::UserAgentHeader, "MultiMC/5.0 (Uncached)");
	QByteArray content(
		"key=public&description=MultiMC5+Log+File&language=plain&format=json&expire=2592000&paste=" +
		m_text.toPercentEncoding());
	request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
	request.setRawHeader("Content-Length", QByteArray::number(content.size()));

	auto worker = MMC->qnam();
	QNetworkReply *rep = worker->post(request, content);

	m_reply = std::shared_ptr<QNetworkReply>(rep);
	setStatus(tr("Uploading to paste.ee"));
	connect(rep, &QNetworkReply::downloadProgress, [&](qint64 value, qint64 max)
	{ setProgress(value / qMax((qint64)1, max) * 100); });
	connect(rep, SIGNAL(error(QNetworkReply::NetworkError)), this,
			SLOT(downloadError(QNetworkReply::NetworkError)));
	connect(rep, SIGNAL(finished()), this, SLOT(downloadFinished()));
}

void PasteUpload::downloadError(QNetworkReply::NetworkError error)
{
	// error happened during download.
	QLOG_ERROR() << "Network error: " << error;
	emitFailed(m_reply->errorString());
}

void PasteUpload::downloadFinished()
{
	// if the download succeeded
	if (m_reply->error() == QNetworkReply::NetworkError::NoError)
	{
		QByteArray data = m_reply->readAll();
		m_reply.reset();
		QJsonParseError jsonError;
		QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
		if (jsonError.error != QJsonParseError::NoError)
		{
			emitFailed(jsonError.errorString());
			return;
		}
		if (!parseResult(doc))
		{
			emitFailed(tr("paste.ee returned an error. Please consult the logs for more information"));
			return;
		}
	}
	// else the download failed
	else
	{
		emitFailed(QString("Network error: %1").arg(m_reply->errorString()));
		m_reply.reset();
		return;
	}
	emitSucceeded();
}

bool PasteUpload::parseResult(QJsonDocument doc)
{
	auto object = doc.object();
	auto status = object.value("status").toString("error");
	if (status == "error")
	{
		QLOG_ERROR() << "paste.ee reported error:" << QString(object.value("error").toString());
		return false;
	}
	m_pasteLink = object.value("paste").toObject().value("link").toString();
	m_pasteID = object.value("paste").toObject().value("id").toString();
	return true;
}

