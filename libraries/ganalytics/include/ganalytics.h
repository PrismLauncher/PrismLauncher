#pragma once

#include <QObject>
#include <QVariantMap>

class QNetworkAccessManager;
class GAnalyticsWorker;

class GAnalytics : public QObject
{
	Q_OBJECT
	Q_ENUMS(LogLevel)

public:
	explicit GAnalytics(const QString &trackingID, const QString &clientID, QObject *parent = 0);
	~GAnalytics();

public:
	enum LogLevel
	{
		Debug,
		Info,
		Error
	};

	void setLogLevel(LogLevel logLevel);
	LogLevel logLevel() const;

	// Getter and Setters
	void setViewportSize(const QString &viewportSize);
	QString viewportSize() const;

	void setLanguage(const QString &language);
	QString language() const;

	void setAnonymizeIPs(bool anonymize);
	bool anonymizeIPs();

	void setSendInterval(int milliseconds);
	int sendInterval() const;

	void startSending();
	bool isSending() const;

	/// Get or set the network access manager. If none is set, the class creates its own on the first request
	void setNetworkAccessManager(QNetworkAccessManager *networkAccessManager);
	QNetworkAccessManager *networkAccessManager() const;

public slots:
	void sendScreenView(const QString &screenName, const QVariantMap &customValues = QVariantMap());
	void sendEvent(const QString &category, const QString &action, const QString &label = QString(), const QVariant &value = QVariant(),
				   const QVariantMap &customValues = QVariantMap());
	void sendException(const QString &exceptionDescription, bool exceptionFatal = true, const QVariantMap &customValues = QVariantMap());
	void startSession();
	void endSession();

signals:
	void isSendingChanged(bool isSending);

private:
	GAnalyticsWorker *d;

	friend QDataStream &operator<<(QDataStream &outStream, const GAnalytics &analytics);
	friend QDataStream &operator>>(QDataStream &inStream, GAnalytics &analytics);
};

QDataStream &operator<<(QDataStream &outStream, const GAnalytics &analytics);
QDataStream &operator>>(QDataStream &inStream, GAnalytics &analytics);
