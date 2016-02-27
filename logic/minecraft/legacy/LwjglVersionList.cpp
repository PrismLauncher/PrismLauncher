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

#include "LwjglVersionList.h"
#include "Env.h"

#include <QtNetwork>
#include <QtXml>
#include <QRegExp>

#include <QDebug>

#define RSS_URL "http://sourceforge.net/projects/java-game-lib/rss"

LWJGLVersionList::LWJGLVersionList(QObject *parent) : BaseVersionList(parent)
{
	setLoading(false);
}

QVariant LWJGLVersionList::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (index.row() > count())
		return QVariant();

	const PtrLWJGLVersion version = m_vlist.at(index.row());

	switch (role)
	{
	case Qt::DisplayRole:
		return version->name();

	case Qt::ToolTipRole:
		return version->url();

	default:
		return QVariant();
	}
}

QVariant LWJGLVersionList::headerData(int section, Qt::Orientation orientation, int role) const
{
	switch (role)
	{
	case Qt::DisplayRole:
		return tr("Version");

	case Qt::ToolTipRole:
		return tr("LWJGL version name.");

	default:
		return QVariant();
	}
}

int LWJGLVersionList::columnCount(const QModelIndex &parent) const
{
	return 1;
}

bool LWJGLVersionList::isLoading() const
{
	return m_loading;
}

void LWJGLVersionList::loadList()
{
	Q_ASSERT_X(!m_loading, "loadList", "list is already loading (m_loading is true)");

	setLoading(true);
	auto worker = ENV.qnam();
	QNetworkRequest req(QUrl(RSS_URL));
	req.setRawHeader("Accept", "application/rss+xml, text/xml, */*");
	req.setRawHeader("User-Agent", "MultiMC/5.0 (Uncached)");
	reply = worker->get(req);
	connect(reply, SIGNAL(finished()), SLOT(netRequestComplete()));
}

inline QDomElement getDomElementByTagName(QDomElement parent, QString tagname)
{
	QDomNodeList elementList = parent.elementsByTagName(tagname);
	if (elementList.count())
		return elementList.at(0).toElement();
	else
		return QDomElement();
}

void LWJGLVersionList::netRequestComplete()
{
	if (reply->error() == QNetworkReply::NoError)
	{
		QRegExp lwjglRegex("lwjgl-(([0-9]\\.?)+)\\.zip");
		Q_ASSERT_X(lwjglRegex.isValid(), "load LWJGL list", "LWJGL regex is invalid");

		QDomDocument doc;

		QString xmlErrorMsg;
		int errorLine;
		auto rawData = reply->readAll();
		if (!doc.setContent(rawData, false, &xmlErrorMsg, &errorLine))
		{
			failed("Failed to load LWJGL list. XML error: " + xmlErrorMsg + " at line " +
				   QString::number(errorLine));
			setLoading(false);
			return;
		}

		QDomNodeList items = doc.elementsByTagName("item");

		QList<PtrLWJGLVersion> tempList;

		for (int i = 0; i < items.length(); i++)
		{
			Q_ASSERT_X(items.at(i).isElement(), "load LWJGL list",
					   "XML element isn't an element... wat?");

			QDomElement linkElement = getDomElementByTagName(items.at(i).toElement(), "link");
			if (linkElement.isNull())
			{
				qDebug() << "Link element" << i << "in RSS feed doesn't exist! Skipping.";
				continue;
			}

			QString link = linkElement.text();

			// Make sure it's a download link.
			if (link.endsWith("/download") && link.contains(lwjglRegex))
			{
				QString name = link.mid(lwjglRegex.indexIn(link) + 6);
				// Subtract 4 here to remove the .zip file extension.
				name = name.left(lwjglRegex.matchedLength() - 10);

				QUrl url(link);
				if (!url.isValid())
				{
					qWarning() << "LWJGL version URL isn't valid:" << link << "Skipping.";
					continue;
				}
				qDebug() << "Discovered LWGL version" << name << "at" << link;
				tempList.append(std::make_shared<LWJGLVersion>(name, link));
			}
		}

		beginResetModel();
		m_vlist.swap(tempList);
		endResetModel();

		qDebug() << "Loaded LWJGL list.";
		finished();
	}
	else
	{
		failed("Failed to load LWJGL list. Network error: " + reply->errorString());
	}

	setLoading(false);
	reply->deleteLater();
}

void LWJGLVersionList::failed(QString msg)
{
	qCritical() << msg;
	emit loadListFailed(msg);
}

void LWJGLVersionList::finished()
{
	emit loadListFinished();
}

void LWJGLVersionList::setLoading(bool loading)
{
	m_loading = loading;
	emit loadingStateUpdated(m_loading);
}
