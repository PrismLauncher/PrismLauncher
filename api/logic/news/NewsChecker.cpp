/* Copyright 2013-2018 MultiMC Contributors
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

#include <QDebug>

NewsChecker::NewsChecker(const QString& feedUrl)
{
	m_feedUrl = feedUrl;
}

void NewsChecker::reloadNews()
{
	// Start a netjob to download the RSS feed and call rssDownloadFinished() when it's done.
	if (isLoadingNews())
	{
		qDebug() << "Ignored request to reload news. Currently reloading already.";
		return;
	}

	qDebug() << "Reloading news.";

	NetJob* job = new NetJob("News RSS Feed");
	job->addNetAction(Net::Download::makeByteArray(m_feedUrl, &newsData));
	QObject::connect(job, &NetJob::succeeded, this, &NewsChecker::rssDownloadFinished);
	QObject::connect(job, &NetJob::failed, this, &NewsChecker::rssDownloadFailed);
	m_newsNetJob.reset(job);
	job->start();
}

void NewsChecker::rssDownloadFinished()
{
	// Parse the XML file and process the RSS feed entries.
	qDebug() << "Finished loading RSS feed.";

	m_newsNetJob.reset();
	QDomDocument doc;
	{
		// Stuff to store error info in.
		QString errorMsg = "Unknown error.";
		int errorLine = -1;
		int errorCol = -1;

		// Parse the XML.
		if (!doc.setContent(newsData, false, &errorMsg, &errorLine, &errorCol))
		{
			QString fullErrorMsg = QString("Error parsing RSS feed XML. %s at %d:%d.").arg(errorMsg, errorLine, errorCol);
			fail(fullErrorMsg);
			newsData.clear();
			return;
		}
		newsData.clear();
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
			qDebug() << "Loaded news entry" << entry->title;
			m_newsEntries.append(entry);
		}
		else
		{
			qWarning() << "Failed to load news entry at index" << i << ":" << errorMsg;
		}
	}

	succeed();
}

void NewsChecker::rssDownloadFailed(QString reason)
{
	// Set an error message and fail.
	fail(tr("Failed to load news RSS feed:\n%1").arg(reason));
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
	qDebug() << "News loading succeeded.";
	m_newsNetJob.reset();
	emit newsLoaded();
}

void NewsChecker::fail(const QString& errorMsg)
{
	m_lastLoadError = errorMsg;
	qDebug() << "Failed to load news:" << errorMsg;
	m_newsNetJob.reset();
	emit newsLoadingFailed(errorMsg);
}

