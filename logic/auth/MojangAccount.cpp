/* Copyright 2013 MultiMC Contributors
 *
 * Authors: Orochimarufan <orochimarufan.x3@gmail.com>
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

#include "MojangAccount.h"

#include <QUuid>

MojangAccount::MojangAccount(const QString& username, QObject* parent) :
	QObject(parent)
{
	// Generate a client token.
	m_clientToken = QUuid::createUuid().toString();

	m_username = username;
}

MojangAccount::MojangAccount(const QString& username, const QString& clientToken, 
							 const QString& accessToken, QObject* parent) :
	QObject(parent)
{
	m_username = username;
	m_clientToken = clientToken;
	m_accessToken = accessToken;
}


QString MojangAccount::username() const
{
	return m_username;
}

QString MojangAccount::clientToken() const
{
	return m_clientToken;
}

QString MojangAccount::accessToken() const
{
	return m_accessToken;
}

