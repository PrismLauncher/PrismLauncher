#pragma once

#include <QUrlQuery>
#include <QDateTime>
#include <QTimer>
#include <QNetworkRequest>
#include <QQueue>

struct QueryBuffer
{
	QUrlQuery postQuery;
	QDateTime time;
};

class GAnalyticsWorker : public QObject
{
	Q_OBJECT

public:
	explicit GAnalyticsWorker(GAnalytics *parent = 0);

	GAnalytics *q;

	QNetworkAccessManager *networkManager = nullptr;

	QQueue<QueryBuffer> m_messageQueue;
	QTimer m_timer;
	QNetworkRequest m_request;
	GAnalytics::LogLevel m_logLevel;

	QString m_trackingID;
	QString m_clientID;
	QString m_userID;
	QString m_appName;
	QString m_appVersion;
	QString m_language;
	QString m_screenResolution;
	QString m_viewportSize;

	bool m_anonymizeIPs = false;
	bool m_isSending = false;

	const static int fourHours = 4 * 60 * 60 * 1000;
	const static QLatin1String dateTimeFormat;

public:
	void logMessage(GAnalytics::LogLevel level, const QString &message);

	QUrlQuery buildStandardPostQuery(const QString &type);
	QString getScreenResolution();
	QString getUserAgent();
	QList<QString> persistMessageQueue();
	void readMessagesFromFile(const QList<QString> &dataList);

	void enqueQueryWithCurrentTime(const QUrlQuery &query);
	void setIsSending(bool doSend);

public slots:
	void postMessage();
	void postMessageFinished();
};

