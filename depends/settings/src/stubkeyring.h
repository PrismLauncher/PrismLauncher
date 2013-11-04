/* Copyright 2013 MultiMC Contributors
 *
 * Authors: Orochimarufan <orochimarufan.x3@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "include/keyring.h"

#include <QSettings>

class StubKeyring : public Keyring
{
public:
	virtual bool storePassword(QString service, QString username, QString password);
	virtual QString getPassword(QString service, QString username);
	virtual bool hasPassword(QString service, QString username);
	virtual QStringList getStoredAccounts(QString service);
	virtual void removeStoredAccount(QString service, QString username);

private:
	friend class Keyring;
	explicit StubKeyring();
	virtual bool isValid()
	{
		return true;
	}

	QSettings m_settings;
};
