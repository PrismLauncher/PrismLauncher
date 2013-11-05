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

#include "S3ListBucket.h"
#include "MultiMC.h"
#include "logger/QsLog.h"
#include <QUrlQuery>
#include <qxmlstream.h>
#include <QDomDocument>

inline QDomElement getDomElementByTagName(QDomElement parent, QString tagname)
{
	QDomNodeList elementList = parent.elementsByTagName(tagname);
	if (elementList.count())
		return elementList.at(0).toElement();
	else
		return QDomElement();
}

S3ListBucket::S3ListBucket(QUrl url) : NetAction()
{
	m_url = url;
	m_status = Job_NotStarted;
}

void S3ListBucket::start()
{
	QUrl finalUrl = m_url;
	if (current_marker.size())
	{
		QUrlQuery query;
		query.addQueryItem("marker", current_marker);
		finalUrl.setQuery(query);
	}
	QNetworkRequest request(finalUrl);
	request.setHeader(QNetworkRequest::UserAgentHeader, "MultiMC/5.0 (Uncached)");
	auto worker = MMC->qnam();
	QNetworkReply *rep = worker->get(request);

	m_reply = std::shared_ptr<QNetworkReply>(rep);
	connect(rep, SIGNAL(downloadProgress(qint64, qint64)),
			SLOT(downloadProgress(qint64, qint64)));
	connect(rep, SIGNAL(finished()), SLOT(downloadFinished()));
	connect(rep, SIGNAL(error(QNetworkReply::NetworkError)),
			SLOT(downloadError(QNetworkReply::NetworkError)));
	connect(rep, SIGNAL(readyRead()), SLOT(downloadReadyRead()));
}

void S3ListBucket::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	emit progress(index_within_job, bytesSoFar + bytesReceived, bytesSoFar + bytesTotal);
}

void S3ListBucket::downloadError(QNetworkReply::NetworkError error)
{
	// error happened during download.
	QLOG_ERROR() << "Error getting URL:" << m_url.toString().toLocal8Bit()
				 << "Network error: " << error;
	m_status = Job_Failed;
}

void S3ListBucket::processValidReply()
{
	QLOG_TRACE() << "GOT: " << m_url.toString() << " marker:" << current_marker;
	auto readContents = [&](QXmlStreamReader & xml)
	{
		QString Key, ETag, Size;
		while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "Contents"))
		{
			if (xml.tokenType() == QXmlStreamReader::StartElement)
			{
				if (xml.name() == "Key")
				{
					Key = xml.readElementText();
				}
				if (xml.name() == "ETag")
				{
					ETag = xml.readElementText();
				}
				if (xml.name() == "Size")
				{
					Size = xml.readElementText();
				}
			}
			xml.readNext();
		}
		if (xml.error() != QXmlStreamReader::NoError)
			return;
		objects.append({Key, ETag, Size.toLongLong()});
	};

	// nothing went wrong...
	QString prefix("http://s3.amazonaws.com/Minecraft.Resources/");
	QByteArray ba = m_reply->readAll();

	QString xmlErrorMsg;

	bool is_truncated = false;
	QXmlStreamReader xml(ba);
	while (!xml.atEnd() && !xml.hasError())
	{
		/* Read next element.*/
		QXmlStreamReader::TokenType token = xml.readNext();
		/* If token is just StartDocument, we'll go to next.*/
		if (token == QXmlStreamReader::StartDocument)
		{
			continue;
		}
		if (token == QXmlStreamReader::StartElement)
		{
			/* If it's named person, we'll dig the information from there.*/
			if (xml.name() == "Contents")
			{
				readContents(xml);
			}
			else if (xml.name() == "IsTruncated")
			{
				is_truncated = (xml.readElementText() == "true");
			}
		}
	}
	if (xml.hasError())
	{
		QLOG_ERROR() << "Failed to process s3.amazonaws.com/Minecraft.Resources. XML error:"
					 << xml.errorString() << ba;
		emit failed(index_within_job);
		return;
	}
	if (is_truncated)
	{
		current_marker = objects.last().Key;
		bytesSoFar += m_reply->size();
		m_reply.reset();
		start();
	}
	else
	{
		m_status = Job_Finished;
		m_reply.reset();
		emit succeeded(index_within_job);
	}
	return;
}

void S3ListBucket::downloadFinished()
{
	// if the download succeeded
	if (m_status != Job_Failed)
	{
		processValidReply();
	}
	// else the download failed
	else
	{
		m_reply.reset();
		emit failed(index_within_job);
		return;
	}
}

void S3ListBucket::downloadReadyRead()
{
	// ~_~
}
