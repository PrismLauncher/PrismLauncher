#pragma once
#include "logic/tasks/Task.h"
#include <QMessageBox>
#include <QNetworkReply>
#include <memory>

class PasteUpload : public Task
{
	Q_OBJECT
public:
	PasteUpload(QWidget *window, QString text);
	virtual ~PasteUpload(){};
	QString pasteLink()
	{
		return m_pasteLink;
	}
	QString pasteID()
	{
		return m_pasteID;
	}
protected:
	virtual void executeTask();

private:
	bool parseResult(QJsonDocument doc);
	QString m_text;
	QString m_error;
	QWidget *m_window;
	QString m_pasteID;
	QString m_pasteLink;
	std::shared_ptr<QNetworkReply> m_reply;
public
slots:
	void downloadError(QNetworkReply::NetworkError);
	void downloadFinished();
};
