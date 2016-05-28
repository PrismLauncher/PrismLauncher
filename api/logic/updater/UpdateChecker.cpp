/* Copyright 2013-2015 MultiMC Contributors
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

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QDebug>

#define API_VERSION 0
#define CHANLIST_FORMAT 0

UpdateChecker::UpdateChecker(QString channelListUrl, QString currentChannel, int currentBuild)
{
	m_channelListUrl = channelListUrl;
	m_currentChannel = currentChannel;
	m_currentBuild = currentBuild;
}

QList<UpdateChecker::ChannelListEntry> UpdateChecker::getChannelList() const
{
	return m_channels;
}

bool UpdateChecker::hasChannels() const
{
	return !m_channels.isEmpty();
}

void UpdateChecker::checkForUpdate(QString updateChannel, bool notifyNoUpdate)
{
	qDebug() << "Checking for updates.";

	// If the channel list hasn't loaded yet, load it and defer checking for updates until
	// later.
	if (!m_chanListLoaded)
	{
		qDebug() << "Channel list isn't loaded yet. Loading channel list and deferring "
						"update check.";
		m_checkUpdateWaiting = true;
		m_deferredUpdateChannel = updateChannel;
		updateChanList(notifyNoUpdate);
		return;
	}

	if (m_updateChecking)
	{
		qDebug() << "Ignoring update check request. Already checking for updates.";
		return;
	}

	m_updateChecking = true;

	// Find the desired channel within the channel list and get its repo URL. If if cannot be
	// found, error.
	m_newRepoUrl = "";
	for (ChannelListEntry entry : m_channels)
	{
		if (entry.id == updateChannel)
			m_newRepoUrl = entry.url;
		if (entry.id == m_currentChannel)
			m_currentRepoUrl = entry.url;
	}

	qDebug() << "m_repoUrl = " << m_newRepoUrl;

	// If we didn't find our channel, error.
	if (m_newRepoUrl.isEmpty())
	{
		qCritical() << "m_repoUrl is empty!";
		emit updateCheckFailed();
		return;
	}

	QUrl indexUrl = QUrl(m_newRepoUrl).resolved(QUrl("index.json"));

	auto job = new NetJob("GoUpdate Repository Index");
	job->addNetAction(Net::Download::makeByteArray(indexUrl, &indexData));
	connect(job, &NetJob::succeeded, [this, notifyNoUpdate](){ updateCheckFinished(notifyNoUpdate); });
	connect(job, &NetJob::failed, this, &UpdateChecker::updateCheckFailed);
	indexJob.reset(job);
	job->start();
}

void UpdateChecker::updateCheckFinished(bool notifyNoUpdate)
{
	qDebug() << "Finished downloading repo index. Checking for new versions.";

	QJsonParseError jsonError;
	indexJob.reset();

	QJsonDocument jsonDoc = QJsonDocument::fromJson(indexData, &jsonError);
	indexData.clear();
	if (jsonError.error != QJsonParseError::NoError || !jsonDoc.isObject())
	{
		qCritical() << "Failed to parse GoUpdate repository index. JSON error"
					 << jsonError.errorString() << "at offset" << jsonError.offset;
		m_updateChecking = false;
		return;
	}

	QJsonObject object = jsonDoc.object();

	bool success = false;
	int apiVersion = object.value("ApiVersion").toVariant().toInt(&success);
	if (apiVersion != API_VERSION || !success)
	{
		qCritical() << "Failed to check for updates. API version mismatch. We're using"
					 << API_VERSION << "server has" << apiVersion;
		m_updateChecking = false;
		return;
	}

	qDebug() << "Processing repository version list.";
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
	if (newBuildNumber != m_currentBuild)
	{
		qDebug() << "Found newer version with ID" << newBuildNumber;
		// Update!
		GoUpdate::Status updateStatus;
		updateStatus.updateAvailable = true;
		updateStatus.currentVersionId = m_currentBuild;
		updateStatus.currentRepoUrl = m_currentRepoUrl;
		updateStatus.newVersionId = newBuildNumber;
		updateStatus.newRepoUrl = m_newRepoUrl;
		emit updateAvailable(updateStatus);
	}
	else if (notifyNoUpdate)
	{
		emit noUpdateFound();
	}
	m_updateChecking = false;
}

void UpdateChecker::updateCheckFailed()
{
	qCritical() << "Update check failed for reasons unknown.";
}

void UpdateChecker::updateChanList(bool notifyNoUpdate)
{
	qDebug() << "Loading the channel list.";

	if (m_chanListLoading)
	{
		qDebug() << "Ignoring channel list update request. Already grabbing channel list.";
		return;
	}

	if (m_channelListUrl.isEmpty())
	{
		qCritical() << "Failed to update channel list. No channel list URL set."
					<< "If you'd like to use MultiMC's update system, please pass the channel "
						"list URL to CMake at compile time.";
		return;
	}

	m_chanListLoading = true;
	NetJob *job = new NetJob("Update System Channel List");
	job->addNetAction(Net::Download::makeByteArray(QUrl(m_channelListUrl), &chanlistData));
	connect(job, &NetJob::succeeded, [this, notifyNoUpdate]() { chanListDownloadFinished(notifyNoUpdate); });
	QObject::connect(job, &NetJob::failed, this, &UpdateChecker::chanListDownloadFailed);
	chanListJob.reset(job);
	job->start();
}

void UpdateChecker::chanListDownloadFinished(bool notifyNoUpdate)
{
	chanListJob.reset();

	QJsonParseError jsonError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(chanlistData, &jsonError);
	chanlistData.clear();
	if (jsonError.error != QJsonParseError::NoError)
	{
		// TODO: Report errors to the user.
		qCritical() << "Failed to parse channel list JSON:" << jsonError.errorString() << "at" << jsonError.offset;
		m_chanListLoading = false;
		return;
	}

	QJsonObject object = jsonDoc.object();

	bool success = false;
	int formatVersion = object.value("format_version").toVariant().toInt(&success);
	if (formatVersion != CHANLIST_FORMAT || !success)
	{
		qCritical()
			<< "Failed to check for updates. Channel list format version mismatch. We're using"
			<< CHANLIST_FORMAT << "server has" << formatVersion;
		m_chanListLoading = false;
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
			qCritical() << "Channel list entry with empty ID, name, or URL. Skipping.";
			continue;
		}
		loadedChannels.append(entry);
	}

	// Swap  the channel list we just loaded into the object's channel list.
	m_channels.swap(loadedChannels);

	m_chanListLoading = false;
	m_chanListLoaded = true;
	qDebug() << "Successfully loaded UpdateChecker channel list.";

	// If we're waiting to check for updates, do that now.
	if (m_checkUpdateWaiting)
		checkForUpdate(m_deferredUpdateChannel, notifyNoUpdate);

	emit channelListLoaded();
}

void UpdateChecker::chanListDownloadFailed(QString reason)
{
	m_chanListLoading = false;
	qCritical() << QString("Failed to download channel list: %1").arg(reason);
	emit channelListLoaded();
}

