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
	QMessageBox *messageBox() const
	{
		return m_messageBox;
	}

protected:
	virtual void executeTask();

private:
	bool parseResult(QJsonDocument doc, QString *parseError);
	QString m_text;
	QString m_error;
	QWidget *m_window;
	QMessageBox *m_messageBox;
	std::shared_ptr<QNetworkReply> m_reply;
public
slots:
	void downloadError(QNetworkReply::NetworkError);
	void downloadFinished();
};
