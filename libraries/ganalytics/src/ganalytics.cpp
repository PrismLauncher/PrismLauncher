#include "ganalytics.h"
#include "ganalytics_worker.h"
#include "sys.h"

#include <QDataStream>
#include <QDebug>
#include <QLocale>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QQueue>
#include <QSettings>
#include <QTimer>
#include <QUrlQuery>
#include <QUuid>

GAnalytics::GAnalytics(const QString &trackingID, const QString &clientID, QObject *parent) : QObject(parent)
{
	d = new GAnalyticsWorker(this);
	d->m_trackingID = trackingID;
	d->m_clientID = clientID;
}

/**
 * Destructor of class GAnalytics.
 */
GAnalytics::~GAnalytics()
{
	delete d;
}

void GAnalytics::setLogLevel(GAnalytics::LogLevel logLevel)
{
	d->m_logLevel = logLevel;
}

GAnalytics::LogLevel GAnalytics::logLevel() const
{
	return d->m_logLevel;
}

// SETTER and GETTER
void GAnalytics::setViewportSize(const QString &viewportSize)
{
	d->m_viewportSize = viewportSize;
}

QString GAnalytics::viewportSize() const
{
	return d->m_viewportSize;
}

void GAnalytics::setLanguage(const QString &language)
{
	d->m_language = language;
}

QString GAnalytics::language() const
{
	return d->m_language;
}

void GAnalytics::setSendInterval(int milliseconds)
{
	d->m_timer.setInterval(milliseconds);
}

int GAnalytics::sendInterval() const
{
	return (d->m_timer.interval());
}

void GAnalytics::startSending()
{
	if (!isSending())
		d->postMessage();
}

bool GAnalytics::isSending() const
{
	return d->m_isSending;
}

void GAnalytics::setNetworkAccessManager(QNetworkAccessManager *networkAccessManager)
{
	if (d->networkManager != networkAccessManager)
	{
		// Delete the old network manager if it was our child
		if (d->networkManager && d->networkManager->parent() == this)
		{
			d->networkManager->deleteLater();
		}

		d->networkManager = networkAccessManager;
	}
}

QNetworkAccessManager *GAnalytics::networkAccessManager() const
{
	return d->networkManager;
}

static void appendCustomValues(QUrlQuery &query, const QVariantMap &customValues)
{
	for (QVariantMap::const_iterator iter = customValues.begin(); iter != customValues.end(); ++iter)
	{
		query.addQueryItem(iter.key(), iter.value().toString());
	}
}

/**
 * Sent screen view is called when the user changed the applications view.
 * These action of the user should be noticed and reported. Therefore
 * a QUrlQuery is build in this method. It holts all the parameter for
 * a http POST. The UrlQuery will be stored in a message Queue.
 */
void GAnalytics::sendScreenView(const QString &screenName, const QVariantMap &customValues)
{
	d->logMessage(Info, QString("ScreenView: %1").arg(screenName));

	QUrlQuery query = d->buildStandardPostQuery("screenview");
	query.addQueryItem("cd", screenName);
	query.addQueryItem("an", d->m_appName);
	query.addQueryItem("av", d->m_appVersion);
	appendCustomValues(query, customValues);

	d->enqueQueryWithCurrentTime(query);
}

/**
 * This method is called whenever a button was pressed in the application.
 * A query for a POST message will be created to report this event. The
 * created query will be stored in a message queue.
 */
void GAnalytics::sendEvent(const QString &category, const QString &action, const QString &label, const QVariant &value, const QVariantMap &customValues)
{
	QUrlQuery query = d->buildStandardPostQuery("event");
	query.addQueryItem("an", d->m_appName);
	query.addQueryItem("av", d->m_appVersion);
	query.addQueryItem("ec", category);
	query.addQueryItem("ea", action);
	if (!label.isEmpty())
		query.addQueryItem("el", label);
	if (value.isValid())
		query.addQueryItem("ev", value.toString());

	appendCustomValues(query, customValues);

	d->enqueQueryWithCurrentTime(query);
}

/**
 * Method is called after an exception was raised. It builds a
 * query for a POST message. These query will be stored in a
 * message queue.
 */
void GAnalytics::sendException(const QString &exceptionDescription, bool exceptionFatal, const QVariantMap &customValues)
{
	QUrlQuery query = d->buildStandardPostQuery("exception");
	query.addQueryItem("an", d->m_appName);
	query.addQueryItem("av", d->m_appVersion);

	query.addQueryItem("exd", exceptionDescription);

	if (exceptionFatal)
	{
		query.addQueryItem("exf", "1");
	}
	else
	{
		query.addQueryItem("exf", "0");
	}
	appendCustomValues(query, customValues);

	d->enqueQueryWithCurrentTime(query);
}

/**
 * Session starts. This event will be sent by a POST message.
 * Query is setup in this method and stored in the message
 * queue.
 */
void GAnalytics::startSession()
{
	QVariantMap customValues;
	customValues.insert("sc", "start");
	sendEvent("Session", "Start", QString(), QVariant(), customValues);
}

/**
 * Session ends. This event will be sent by a POST message.
 * Query is setup in this method and stored in the message
 * queue.
 */
void GAnalytics::endSession()
{
	QVariantMap customValues;
	customValues.insert("sc", "end");
	sendEvent("Session", "End", QString(), QVariant(), customValues);
}

/**
 * Qut stream to persist class GAnalytics.
 */
QDataStream &operator<<(QDataStream &outStream, const GAnalytics &analytics)
{
	outStream << analytics.d->persistMessageQueue();

	return outStream;
}

/**
 * In stream to read GAnalytics from file.
 */
QDataStream &operator>>(QDataStream &inStream, GAnalytics &analytics)
{
	QList<QString> dataList;
	inStream >> dataList;
	analytics.d->readMessagesFromFile(dataList);

	return inStream;
}
