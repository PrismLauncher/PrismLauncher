#pragma once

#include <QFile>
#include <QtNetwork/QtNetwork>
#include <memory>
#include <minecraft/auth/AuthSession.h>
#include "tasks/Task.h"
#include "multimc_logic_export.h"

typedef std::shared_ptr<class SkinUpload> SkinUploadPtr;

class MULTIMC_LOGIC_EXPORT SkinUpload : public Task
{
	Q_OBJECT
public:
	enum Model
	{
		STEVE,
		ALEX
	};

	// Note this class takes ownership of the file.
	SkinUpload(QObject *parent, AuthSessionPtr session, QByteArray skin, Model model = STEVE);
	virtual ~SkinUpload() {}

private:
	Model m_model;
	QByteArray m_skin;
	AuthSessionPtr m_session;
	std::shared_ptr<QNetworkReply> m_reply;
protected:
	virtual void executeTask();

public slots:

	void downloadError(QNetworkReply::NetworkError);

	void downloadFinished();
};
