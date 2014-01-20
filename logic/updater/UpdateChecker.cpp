/* Copyright 2013 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "UpdateChecker.h"

#include "MultiMC.h"

#include "logger/QsLog.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

#include <settingsobject.h>

#define API_VERSION 0
#define CHANLIST_FORMAT 0

UpdateChecker::UpdateChecker()
{
	m_channelListUrl = CHANLIST_URL;
	m_updateChecking = false;
	m_chanListLoading = false;
	m_checkUpdateWaiting = false;
	m_chanListLoaded = false;
}

QList<UpdateChecker::ChannelListEntry> UpdateChecker::getChannelList() const
{
	return m_channels;
}

bool UpdateChecker::hasChannels() const
{
	return !m_channels.isEmpty();
}

void UpdateChecker::checkForUpdate(bool notifyNoUpdate)
{
	QLOG_DEBUG() << "Checking for updates.";

	// If the channel list hasn't loaded yet, load it and defer checking for updates until
	// later.
	if (!m_chanListLoaded)
	{
		QLOG_DEBUG() << "Channel list isn't loaded yet. Loading channel list and deferring "
						"update check.";
		m_checkUpdateWaiting = true;
		updateChanList();
		return;
	}

	if (m_updateChecking)
	{
		QLOG_DEBUG() << "Ignoring update check request. Already checking for updates.";
		return;
	}

	m_updateChecking = true;

	// Get the channel we're checking.
	QString updateChannel = MMC->settings()->get("UpdateChannel").toString();

	// Find the desired channel within the channel list and get its repo URL. If if cannot be
	// found, error.
	m_repoUrl = "";
	for (ChannelListEntry entry : m_channels)
	{
		if (entry.id == updateChannel)
			m_repoUrl = entry.url;
	}

	// If we didn't find our channel, error.
	if (m_repoUrl.isEmpty())
	{
		emit updateCheckFailed();
		return;
	}

	QUrl indexUrl = QUrl(m_repoUrl).resolved(QUrl("index.json"));

	auto job = new NetJob("GoUpdate Repository Index");
	job->addNetAction(ByteArrayDownload::make(indexUrl));
	connect(job, &NetJob::succeeded, [this, notifyNoUpdate]()
	{ updateCheckFinished(notifyNoUpdate); });
	connect(job, SIGNAL(failed()), SLOT(updateCheckFailed()));
	indexJob.reset(job);
	job->start();
}

void UpdateChecker::updateCheckFinished(bool notifyNoUpdate)
{
	QLOG_DEBUG() << "Finished downloading repo index. Checking for new versions.";

	QJsonParseError jsonError;
	QByteArray data;
	{
		ByteArrayDownloadPtr dl =
			std::dynamic_pointer_cast<ByteArrayDownload>(indexJob->first());
		data = dl->m_data;
		indexJob.reset();
	}

	QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);
	if (jsonError.error != QJsonParseError::NoError || !jsonDoc.isObject())
	{
		QLOG_ERROR() << "Failed to parse GoUpdate repository index. JSON error"
					 << jsonError.errorString() << "at offset" << jsonError.offset;
		return;
	}

	QJsonObject object = jsonDoc.object();

	bool success = false;
	int apiVersion = object.value("ApiVersion").toVariant().toInt(&success);
	if (apiVersion != API_VERSION || !success)
	{
		QLOG_ERROR() << "Failed to check for updates. API version mismatch. We're using"
					 << API_VERSION << "server has" << apiVersion;
		return;
	}

	QLOG_DEBUG() << "Processing repository version list.";
	QJsonObject newestVersion;
	QJsonArray versions = object.value("Versions").toArray();
	for (QJsonValue versionVal : versions)
	{
		QJsonObject version = versionVal.toObject();
		if (newestVersion.value("Id").toVariant().toInt() <
			version.value("Id").toVariant().toInt())
		{
			newestVersion = version;
		}
	}

	// We've got the version with the greatest ID number. Now compare it to our current build
	// number and update if they're different.
	int newBuildNumber = newestVersion.value("Id").toVariant().toInt();
	if (newBuildNumber != MMC->version().build)
	{
		QLOG_DEBUG() << "Found newer version with ID" << newBuildNumber;
		// Update!
		emit updateAvailable(m_repoUrl, newestVersion.value("Name").toVariant().toString(),
							 newBuildNumber);
	}
	else if (notifyNoUpdate)
	{
		emit noUpdateFound();
	}

	m_updateChecking = false;
}

void UpdateChecker::updateCheckFailed()
{
	// TODO: log errors better
	QLOG_ERROR() << "Update check failed for reasons unknown.";
}

void UpdateChecker::updateChanList()
{
	QLOG_DEBUG() << "Loading the channel list.";

	if (m_channelListUrl.isEmpty())
	{
		QLOG_ERROR() << "Failed to update channel list. No channel list URL set."
					 << "If you'd like to use MultiMC's update system, please pass the channel "
						"list URL to CMake at compile time.";
		return;
	}

	m_chanListLoading = true;
	NetJob *job = new NetJob("Update System Channel List");
	job->addNetAction(ByteArrayDownload::make(QUrl(m_channelListUrl)));
	QObject::connect(job, &NetJob::succeeded, this, &UpdateChecker::chanListDownloadFinished);
	QObject::connect(job, &NetJob::failed, this, &UpdateChecker::chanListDownloadFailed);
	chanListJob.reset(job);
	job->start();
}

void UpdateChecker::chanListDownloadFinished()
{
	QByteArray data;
	{
		ByteArrayDownloadPtr dl =
			std::dynamic_pointer_cast<ByteArrayDownload>(chanListJob->first());
		data = dl->m_data;
		chanListJob.reset();
	}

	QJsonParseError jsonError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);
	if (jsonError.error != QJsonParseError::NoError)
	{
		// TODO: Report errors to the user.
		QLOG_ERROR() << "Failed to parse channel list JSON:" << jsonError.errorString() << "at"
					 << jsonError.offset;
		return;
	}

	QJsonObject object = jsonDoc.object();

	bool success = false;
	int formatVersion = object.value("format_version").toVariant().toInt(&success);
	if (formatVersion != CHANLIST_FORMAT || !success)
	{
		QLOG_ERROR()
			<< "Failed to check for updates. Channel list format version mismatch. We're using"
			<< CHANLIST_FORMAT << "server has" << formatVersion;
		return;
	}

	// Load channels into a temporary array.
	QList<ChannelListEntry> loadedChannels;
	QJsonArray channelArray = object.value("channels").toArray();
	for (QJsonValue chanVal : channelArray)
	{
		QJsonObject channelObj = chanVal.toObject();
		ChannelListEntry entry{channelObj.value("id").toVariant().toString(),
							   channelObj.value("name").toVariant().toString(),
							   channelObj.value("description").toVariant().toString(),
							   channelObj.value("url").toVariant().toString()};
		if (entry.id.isEmpty() || entry.name.isEmpty() || entry.url.isEmpty())
		{
			QLOG_ERROR() << "Channel list entry with empty ID, name, or URL. Skipping.";
			continue;
		}
		loadedChannels.append(entry);
	}

	// Swap  the channel list we just loaded into the object's channel list.
	m_channels.swap(loadedChannels);

	m_chanListLoading = false;
	m_chanListLoaded = true;
	QLOG_INFO() << "Successfully loaded UpdateChecker channel list.";

	// If we're waiting to check for updates, do that now.
	if (m_checkUpdateWaiting)
		checkForUpdate(false);

	emit channelListLoaded();
}

void UpdateChecker::chanListDownloadFailed()
{
	m_chanListLoading = false;
	QLOG_ERROR() << "Failed to download channel list.";
	emit channelListLoaded();
}

