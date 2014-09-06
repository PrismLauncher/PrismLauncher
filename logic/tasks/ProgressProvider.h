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

class ProgressProvider : public QObject
{
	Q_OBJECT
protected:
	explicit ProgressProvider(QObject *parent = 0) : QObject(parent)
	{
	}
signals:
	void started();
	void progress(qint64 current, qint64 total);
	void succeeded();
	void failed(QString reason);
	void status(QString status);

public:
	virtual ~ProgressProvider() {}
	virtual bool isRunning() const = 0;
public
slots:
	virtual void start() = 0;
	virtual void abort() = 0;
};
