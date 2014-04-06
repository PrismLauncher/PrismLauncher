#include "NotificationChecker.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "MultiMC.h"
#include "BuildConfig.h"
#include "logic/net/CacheDownload.h"

NotificationChecker::NotificationChecker(QObject *parent)
	: QObject(parent), m_notificationsUrl(QUrl(BuildConfig.NOTIFICATION_URL))
{
	// this will call checkForNotifications once the event loop is running
	QMetaObject::invokeMethod(this, "checkForNotifications", Qt::QueuedConnection);
}

QUrl NotificationChecker::notificationsUrl() const
{
	return m_notificationsUrl;
}
void NotificationChecker::setNotificationsUrl(const QUrl &notificationsUrl)
{
	m_notificationsUrl = notificationsUrl;
}

QList<NotificationChecker::NotificationEntry> NotificationChecker::notificationEntries() const
{
	return m_entries;
}

void NotificationChecker::checkForNotifications()
{
	if (!m_notificationsUrl.isValid())
	{
		QLOG_ERROR() << "Failed to check for notifications. No notifications URL set."
					 << "If you'd like to use MultiMC's notification system, please pass the "
						"URL to CMake at compile time.";
		return;
	}
	if (m_checkJob)
	{
		return;
	}
	m_checkJob.reset(new NetJob("Checking for notifications"));
	auto entry = MMC->metacache()->resolveEntry("root", "notifications.json");
	entry->stale = true;
	m_checkJob->addNetAction(m_download = CacheDownload::make(m_notificationsUrl, entry));
	connect(m_download.get(), &CacheDownload::succeeded, this,
			&NotificationChecker::downloadSucceeded);
	m_checkJob->start();
}

void NotificationChecker::downloadSucceeded(int)
{
	m_entries.clear();

	QFile file(m_download->getTargetFilepath());
	if (file.open(QFile::ReadOnly))
	{
		QJsonArray root = QJsonDocument::fromJson(file.readAll()).array();
		for (auto it = root.begin(); it != root.end(); ++it)
		{
			QJsonObject obj = (*it).toObject();
			NotificationEntry entry;
			entry.id = obj.value("id").toDouble();
			entry.message = obj.value("message").toString();
			entry.channel = obj.value("channel").toString();
			entry.platform = obj.value("platform").toString();
			entry.from = obj.value("from").toString();
			entry.to = obj.value("to").toString();
			const QString type = obj.value("type").toString("critical");
			if (type == "critical")
			{
				entry.type = NotificationEntry::Critical;
			}
			else if (type == "warning")
			{
				entry.type = NotificationEntry::Warning;
			}
			else if (type == "information")
			{
				entry.type = NotificationEntry::Information;
			}
			m_entries.append(entry);
		}
	}

	m_checkJob.reset();

	emit notificationCheckFinished();
}

bool NotificationChecker::NotificationEntry::applies() const
{
	bool channelApplies = channel.isEmpty() || channel == BuildConfig.VERSION_CHANNEL;
	bool platformApplies = platform.isEmpty() || platform == BuildConfig.BUILD_PLATFORM;
	bool fromApplies =
		from.isEmpty() || from == BuildConfig.FULL_VERSION_STR || !versionLessThan(BuildConfig.FULL_VERSION_STR, from);
	bool toApplies =
		to.isEmpty() || to == BuildConfig.FULL_VERSION_STR || !versionLessThan(to, BuildConfig.FULL_VERSION_STR);
	return channelApplies && platformApplies && fromApplies && toApplies;
}

bool NotificationChecker::NotificationEntry::versionLessThan(const QString &v1,
															 const QString &v2)
{
	QStringList l1 = v1.split('.');
	QStringList l2 = v2.split('.');
	while (!l1.isEmpty() && !l2.isEmpty())
	{
		int one = l1.isEmpty() ? 0 : l1.takeFirst().toInt();
		int two = l2.isEmpty() ? 0 : l2.takeFirst().toInt();
		if (one != two)
		{
			return one < two;
		}
	}
	return false;
}
