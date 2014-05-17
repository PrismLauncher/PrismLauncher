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

	bool isLoadingStatus() const;

	QMap<QString, QString> getStatusEntries() const;

	void Q_SLOT reloadStatus();

protected:
	virtual void timerEvent(QTimerEvent *);

signals:
	void statusLoading(bool loading);
	void statusChanged(QMap<QString, QString> newStatus);

protected slots:
	void statusDownloadFinished();
	void statusDownloadFailed();

protected:
	QMap<QString, QString> m_prevEntries;
	QMap<QString, QString> m_statusEntries;
	NetJobPtr m_statusNetJob;
	QString m_lastLoadError;

	void Q_SLOT succeed();
	void Q_SLOT fail(const QString& errorMsg);
};

