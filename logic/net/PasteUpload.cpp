#include "PasteUpload.h"
#include "MultiMC.h"
#include "logger/QsLog.h"
#include <QJsonObject>
#include <QJsonDocument>
#include "gui/dialogs/CustomMessageBox.h"
#include <QDesktopServices>

PasteUpload::PasteUpload(QWidget *window, QString text) : m_text(text), m_window(window)
{
}

void PasteUpload::executeTask()
{
	QNetworkRequest request(QUrl("http://paste.ee/api"));
	request.setHeader(QNetworkRequest::UserAgentHeader, "MultiMC/5.0 (Uncached)");
	QByteArray content(
		"key=public&description=MultiMC5+Log+File&language=plain&format=json&paste=" +
		m_text.toUtf8());
	request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
	request.setRawHeader("Content-Length", QByteArray::number(content.size()));

	auto worker = MMC->qnam();
	QNetworkReply *rep = worker->post(request, content);

	m_reply = std::shared_ptr<QNetworkReply>(rep);
	connect(rep, &QNetworkReply::downloadProgress, [&](qint64 value, qint64 max)
	{ setProgress(value / max * 100); });
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
		QString error;
		if (!parseResult(doc, &error))
		{
			emitFailed(error);
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

bool PasteUpload::parseResult(QJsonDocument doc, QString *parseError)
{
	auto object = doc.object();
	auto status = object.value("status").toString("error");
	if (status == "error")
	{
		parseError = new QString(object.value("error").toString());
		return false;
	}
	// FIXME: not the place for GUI things.
	QString pasteUrl = object.value("paste").toObject().value("link").toString();
	QDesktopServices::openUrl(pasteUrl);
	return true;
}

