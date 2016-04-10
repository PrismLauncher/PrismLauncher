/* Copyright 2015 MultiMC Contributors
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

class QTimer;

class BaseConfigObject
{
public:
	void setSaveTimeout(int msec);

protected:
	explicit BaseConfigObject(const QString &filename);
	virtual ~BaseConfigObject();

	// cppcheck-suppress pureVirtualCall
	virtual QByteArray doSave() const = 0;
	virtual void doLoad(const QByteArray &data) = 0;

	void setSavingDisabled(bool savingDisabled) { m_disableSaving = savingDisabled; }

	QString fileName() const { return m_filename; }

public:
	void scheduleSave();
	void saveNow();
	void loadNow();

private:
	QTimer *m_saveTimer;
	QTimer *m_initialReadTimer;
	QString m_filename;
	QMetaObject::Connection m_appQuitConnection;
	bool m_disableSaving = false;
};
