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

#include "loginresponse.h"

LoginResponse::LoginResponse(const QString& username, const QString& sessionID, 
							 qint64 latestVersion, QObject *parent) :
	QObject(parent)
{
	this->m_username = username;
	this->m_sessionID = sessionID;
	this->m_latestVersion = latestVersion;
}

LoginResponse::LoginResponse()
{
	this->m_username = "";
	this->m_sessionID = "";
	this->m_latestVersion = 0;
}

LoginResponse::LoginResponse(const LoginResponse &other)
{
	this->m_username = other.username();
	this->m_sessionID = other.sessionID();
	this->m_latestVersion = other.latestVersion();
}

QString LoginResponse::username() const
{
	return m_username;
}

void LoginResponse::setUsername(const QString& username)
{
	this->m_username = username;
}

QString LoginResponse::sessionID() const
{
	return m_sessionID;
}

void LoginResponse::setSessionID(const QString& sessionID)
{
	this->m_sessionID = sessionID;
}

qint64 LoginResponse::latestVersion() const
{
	return m_latestVersion;
}

void LoginResponse::setLatestVersion(qint64 v)
{
	this->m_latestVersion = v;
}
