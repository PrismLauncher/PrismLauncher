#pragma once
#include "tasks/Task.h"
#include <QMessageBox>
#include <QNetworkReply>
#include <memory>

#include "multimc_logic_export.h"

class MULTIMC_LOGIC_EXPORT PasteUpload : public Task
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
	uint32_t maxSize()
	{
		// 2MB for paste.ee
		return 1024*1024*2;
	}
	bool validateText();
protected:
	virtual void executeTask();

private:
	bool parseResult(QJsonDocument doc);
	QByteArray m_text;
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
