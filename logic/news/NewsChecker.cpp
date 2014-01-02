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

#include "NewsChecker.h"

#include <QByteArray>
#include <QDomDocument>

#include <logger/QsLog.h>

NewsChecker::NewsChecker(const QString& feedUrl)
{
	m_feedUrl = feedUrl;
}

void NewsChecker::reloadNews()
{
	// Start a netjob to download the RSS feed and call rssDownloadFinished() when it's done.
	if (isLoadingNews())
	{
		QLOG_INFO() << "Ignored request to reload news. Currently reloading already.";
		return;
	}
	
	QLOG_INFO() << "Reloading news.";

	NetJob* job = new NetJob("News RSS Feed");
	job->addNetAction(ByteArrayDownload::make(m_feedUrl));
	QObject::connect(job, &NetJob::succeeded, this, &NewsChecker::rssDownloadFinished);
	QObject::connect(job, &NetJob::failed, this, &NewsChecker::rssDownloadFailed);
	m_newsNetJob.reset(job);
	job->start();
}

void NewsChecker::rssDownloadFinished()
{
	// Parse the XML file and process the RSS feed entries.
	QLOG_DEBUG() << "Finished loading RSS feed.";

	QByteArray data;
	{
		ByteArrayDownloadPtr dl = std::dynamic_pointer_cast<ByteArrayDownload>(m_newsNetJob->first());
		data = dl->m_data;
		m_newsNetJob.reset();
	}

	QDomDocument doc;
	{
		// Stuff to store error info in.
		QString errorMsg = "Unknown error.";
		int errorLine = -1;
		int errorCol = -1;

		// Parse the XML.
		if (!doc.setContent(data, false, &errorMsg, &errorLine, &errorCol))
		{
			QString fullErrorMsg = QString("Error parsing RSS feed XML. %s at %d:%d.").arg(errorMsg, errorLine, errorCol);
			fail(fullErrorMsg);
			return;
		}
	}

	// If the parsing succeeded, read it.
	QDomNodeList items = doc.elementsByTagName("item");
	m_newsEntries.clear();
	for (int i = 0; i < items.length(); i++)
	{
		QDomElement element = items.at(i).toElement();
		NewsEntryPtr entry;
		entry.reset(new NewsEntry());
		QString errorMsg = "An unknown error occurred.";
		if (NewsEntry::fromXmlElement(element, entry.get(), &errorMsg))
		{
			QLOG_DEBUG() << "Loaded news entry" << entry->title;
			m_newsEntries.append(entry);
		}
		else
		{
			QLOG_WARN() << "Failed to load news entry at index" << i << ":" << errorMsg;
		}
	}

	succeed();
}

void NewsChecker::rssDownloadFailed()
{
	// Set an error message and fail.
	fail("Failed to load news RSS feed.");
}


QList<NewsEntryPtr> NewsChecker::getNewsEntries() const
{
	return m_newsEntries;
}

bool NewsChecker::isLoadingNews() const
{
	return m_newsNetJob.get() != nullptr;
}

QString NewsChecker::getLastLoadErrorMsg() const
{
	return m_lastLoadError;
}

void NewsChecker::succeed()
{
	m_lastLoadError = "";
	QLOG_DEBUG() << "News loading succeeded.";
	m_newsNetJob.reset();
	emit newsLoaded();
}

void NewsChecker::fail(const QString& errorMsg)
{
	m_lastLoadError = errorMsg;
	QLOG_DEBUG() << "Failed to load news:" << errorMsg;
	m_newsNetJob.reset();
	emit newsLoadingFailed(errorMsg);
}

