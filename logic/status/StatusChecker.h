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

#pragma once

#include <QObject>
#include <QString>
#include <QList>

#include <logic/net/NetJob.h>

class StatusChecker : public QObject
{
	Q_OBJECT
public:
	StatusChecker();

	QString getLastLoadErrorMsg() const;

	bool isStatusLoaded() const;
	
	bool isLoadingStatus() const;

	QMap<QString, QString> getStatusEntries() const;

	void Q_SLOT reloadStatus();

signals:
	void statusLoaded();
	void statusLoadingFailed(QString errorMsg);

protected slots:
	void statusDownloadFinished();
	void statusDownloadFailed();

protected:
	QMap<QString, QString> m_statusEntries;
	NetJobPtr m_statusNetJob;

	//! True if news has been loaded.
	bool m_loadedStatus;

	QString m_lastLoadError;

	/*!
	 * Emits newsLoaded() and sets m_lastLoadError to empty string.
	 */
	void Q_SLOT succeed();

	/*!
	 * Emits newsLoadingFailed() and sets m_lastLoadError to the given message.
	 */
	void Q_SLOT fail(const QString& errorMsg);
};

