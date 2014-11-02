/* Copyright 2013-2014 MultiMC Contributors
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

#include "NewsEntry.h"

#include <QDomNodeList>
#include <QVariant>

NewsEntry::NewsEntry(QObject* parent) :
	QObject(parent)
{
	this->title = tr("Untitled");
	this->content = tr("No content.");
	this->link = "";
	this->author = tr("Unknown Author");
	this->pubDate = QDateTime::currentDateTime();
}

NewsEntry::NewsEntry(const QString& title, const QString& content, const QString& link, const QString& author, const QDateTime& pubDate, QObject* parent) :
	QObject(parent)
{
	this->title = title;
	this->content = content;
	this->link = link;
	this->author = author;
	this->pubDate = pubDate;
}

/*!
 * Gets the text content of the given child element as a QVariant.
 */
inline QString childValue(const QDomElement& element, const QString& childName, QString defaultVal="")
{
	QDomNodeList nodes = element.elementsByTagName(childName);
	if (nodes.count() > 0)
	{
		QDomElement element = nodes.at(0).toElement();
		return element.text();
	}
	else
	{
		return defaultVal;
	}
}

bool NewsEntry::fromXmlElement(const QDomElement& element, NewsEntry* entry, QString* errorMsg)
{
	QString title = childValue(element, "title", tr("Untitled"));
	QString content = childValue(element, "description", tr("No content."));
	QString link = childValue(element, "link");
	QString author = childValue(element, "dc:creator", tr("Unknown Author"));
	QString pubDateStr = childValue(element, "pubDate");

	// FIXME: For now, we're just ignoring timezones. We assume that all time zones in the RSS feed are the same.
	QString dateFormat("ddd, dd MMM yyyy hh:mm:ss");
	QDateTime pubDate = QDateTime::fromString(pubDateStr, dateFormat);

	entry->title = title;
	entry->content = content;
	entry->link = link;
	entry->author = author;
	entry->pubDate = pubDate;
	return true;
}

