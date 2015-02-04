#pragma once

#include <QObject>

#include "logic/net/NetJob.h"
#include "logic/net/CacheDownload.h"

class NotificationChecker : public QObject
{
	Q_OBJECT

public:
	explicit NotificationChecker(QObject *parent = 0);

	QUrl notificationsUrl() const;
	void setNotificationsUrl(const QUrl &notificationsUrl);

	struct NotificationEntry
	{
		int id;
		QString message;
		enum
		{
			Critical,
			Warning,
			Information
		} type;
		QString channel;
		QString platform;
		QString from;
		QString to;
		bool applies() const;
		static bool versionLessThan(const QString &v1, const QString &v2);
	};

	QList<NotificationEntry> notificationEntries() const;

public
slots:
	void checkForNotifications();

private
slots:
	void downloadSucceeded(int);

signals:
	void notificationCheckFinished();

private:
	QList<NotificationEntry> m_entries;
	QUrl m_notificationsUrl;
	NetJobPtr m_checkJob;
	CacheDownloadPtr m_download;
};
