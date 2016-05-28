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

#pragma once

#include <QObject>
#include <QString>
#include <QList>

#include <net/NetJob.h>

#include "multimc_logic_export.h"

class MULTIMC_LOGIC_EXPORT StatusChecker : public QObject
{
	Q_OBJECT
public: /* con/des */
	StatusChecker();

public: /* methods */
	QString getLastLoadErrorMsg() const;
	bool isLoadingStatus() const;
	QMap<QString, QString> getStatusEntries() const;

signals:
	void statusLoading(bool loading);
	void statusChanged(QMap<QString, QString> newStatus);

public slots:
	void reloadStatus();

protected: /* methods */
	virtual void timerEvent(QTimerEvent *);

protected slots:
	void statusDownloadFinished();
	void statusDownloadFailed(QString reason);
	void succeed();
	void fail(const QString& errorMsg);

protected: /* data */
	QMap<QString, QString> m_prevEntries;
	QMap<QString, QString> m_statusEntries;
	NetJobPtr m_statusNetJob;
	QString m_lastLoadError;
	QByteArray dataSink;
};

