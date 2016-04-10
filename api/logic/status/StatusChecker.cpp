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

#include "StatusChecker.h"

#include <net/URLConstants.h>

#include <QByteArray>

#include <QDebug>

StatusChecker::StatusChecker()
{

}

void StatusChecker::timerEvent(QTimerEvent *e)
{
	QObject::timerEvent(e);
	reloadStatus();
}

void StatusChecker::reloadStatus()
{
	if (isLoadingStatus())
	{
		// qDebug() << "Ignored request to reload status. Currently reloading already.";
		return;
	}

	// qDebug() << "Reloading status.";

	NetJob* job = new NetJob("Status JSON");
	job->addNetAction(ByteArrayDownload::make(URLConstants::MOJANG_STATUS_URL));
	QObject::connect(job, &NetJob::succeeded, this, &StatusChecker::statusDownloadFinished);
	QObject::connect(job, &NetJob::failed, this, &StatusChecker::statusDownloadFailed);
	m_statusNetJob.reset(job);
	emit statusLoading(true);
	job->start();
}

void StatusChecker::statusDownloadFinished()
{
	qDebug() << "Finished loading status JSON.";
	m_statusEntries.clear();
	QByteArray data;
	{
		ByteArrayDownloadPtr dl = std::dynamic_pointer_cast<ByteArrayDownload>(m_statusNetJob->first());
		data = dl->m_data;
		m_statusNetJob.reset();
	}

	QJsonParseError jsonError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);

	if (jsonError.error != QJsonParseError::NoError)
	{
		fail("Error parsing status JSON:" + jsonError.errorString());
		return;
	}

	if (!jsonDoc.isArray())
	{
		fail("Error parsing status JSON: JSON root is not an array");
		return;
	}

	QJsonArray root = jsonDoc.array();

	for(auto status = root.begin(); status != root.end(); ++status)
	{
		QVariantMap map = (*status).toObject().toVariantMap();

		for (QVariantMap::const_iterator iter = map.begin(); iter != map.end(); ++iter)
		{
			QString key = iter.key();
			QVariant value = iter.value();

			if(value.type() == QVariant::Type::String)
			{
				m_statusEntries.insert(key, value.toString());
				//qDebug() << "Status JSON object: " << key << m_statusEntries[key];
			}
			else
			{
				fail("Malformed status JSON: expected status type to be a string.");
				return;
			}
		}
	}

	succeed();
}

void StatusChecker::statusDownloadFailed(QString reason)
{
	fail(tr("Failed to load status JSON:\n%1").arg(reason));
}


QMap<QString, QString> StatusChecker::getStatusEntries() const
{
	return m_statusEntries;
}

bool StatusChecker::isLoadingStatus() const
{
	return m_statusNetJob.get() != nullptr;
}

QString StatusChecker::getLastLoadErrorMsg() const
{
	return m_lastLoadError;
}

void StatusChecker::succeed()
{
	if(m_prevEntries != m_statusEntries)
	{
		emit statusChanged(m_statusEntries);
		m_prevEntries = m_statusEntries;
	}
	m_lastLoadError = "";
	qDebug() << "Status loading succeeded.";
	m_statusNetJob.reset();
	emit statusLoading(false);
}

void StatusChecker::fail(const QString& errorMsg)
{
	if(m_prevEntries != m_statusEntries)
	{
		emit statusChanged(m_statusEntries);
		m_prevEntries = m_statusEntries;
	}
	m_lastLoadError = errorMsg;
	qDebug() << "Failed to load status:" << errorMsg;
	m_statusNetJob.reset();
	emit statusLoading(false);
}

