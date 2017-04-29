/* Copyright 2013-2017 MultiMC Contributors
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

#include "tasks/Task.h"

#include <QObject>
#include <QUrl>
#include <memory>
#include <QNetworkReply>
#include <QObjectPtr.h>

#include "multimc_logic_export.h"

typedef std::shared_ptr<class NetAction> NetActionPtr;
class MULTIMC_LOGIC_EXPORT NetAction : public Task
{
	Q_OBJECT
protected:
	explicit NetAction(QObject *parent = 0) : Task(parent) {};

public:
	virtual ~NetAction() {};

public:
	unique_qobject_ptr<QNetworkReply> m_reply;
	QUrl m_url;
	// FIXME: pull this up into Task
	Status m_status = Status::NotStarted;

signals:
	void failed();
	void aborted();

protected slots:
	virtual void downloadProgress(qint64 bytesReceived, qint64 bytesTotal) = 0;
	virtual void downloadError(QNetworkReply::NetworkError error) = 0;
	virtual void downloadFinished() = 0;
	virtual void downloadReadyRead() = 0;
};
