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

#include <logger/QsLog.h>

MojangAccount::MojangAccount(const QString& username, QObject* parent) :
	QObject(parent)
{
	// Generate a client token.
	m_clientToken = QUuid::createUuid().toString();

	m_username = username;

	m_currentProfile = -1;
}

MojangAccount::MojangAccount(const QString& username, const QString& clientToken, 
							 const QString& accessToken, QObject* parent) :
	QObject(parent)
{
	m_username = username;
	m_clientToken = clientToken;
	m_accessToken = accessToken;

	m_currentProfile = -1;
}

MojangAccount::MojangAccount(const MojangAccount& other, QObject* parent)
{
	m_username = other.username();
	m_clientToken = other.clientToken();
	m_accessToken = other.accessToken();

	m_profiles = other.m_profiles;
	m_currentProfile = other.m_currentProfile;
}


QString MojangAccount::username() const
{
	return m_username;
}

QString MojangAccount::clientToken() const
{
	return m_clientToken;
}

void MojangAccount::setClientToken(const QString& clientToken)
{
	m_clientToken = clientToken;
}


QString MojangAccount::accessToken() const
{
	return m_accessToken;
}

void MojangAccount::setAccessToken(const QString& accessToken)
{
	m_accessToken = accessToken;
}


const QList<AccountProfile> MojangAccount::profiles() const
{
	return m_profiles;
}

const AccountProfile* MojangAccount::currentProfile() const
{
	if (m_currentProfile < 0)
		return nullptr;
	else
		return &m_profiles.at(m_currentProfile);
}

bool MojangAccount::setProfile(const QString& profileId)
{
	const QList<AccountProfile>& profiles = this->profiles();
	for (int i = 0; i < profiles.length(); i++)
	{
		if (profiles.at(i).id() == profileId)
		{
			m_currentProfile = i;
			return true;
		}
	}
	return false;
}

void MojangAccount::loadProfiles(const ProfileList& profiles)
{
	m_profiles.clear();
	for (auto profile : profiles)
		m_profiles.append(profile);
}

MojangAccountPtr MojangAccount::loadFromJson(const QJsonObject& object)
{
	// The JSON object must at least have a username for it to be valid.
	if (!object.value("username").isString())
	{
		QLOG_ERROR() << "Can't load Mojang account info from JSON object. Username field is missing or of the wrong type.";
		return nullptr;
	}
	
	QString username = object.value("username").toString("");
	QString clientToken = object.value("clientToken").toString("");
	QString accessToken = object.value("accessToken").toString("");

	// TODO: Load profiles?

	return MojangAccountPtr(new MojangAccount(username, clientToken, accessToken));
}

QJsonObject MojangAccount::saveToJson()
{
	QJsonObject json;
	json.insert("username", username());
	json.insert("clientToken", clientToken());
	json.insert("accessToken", accessToken());
	// TODO: Save profiles?
	return json;
}


AccountProfile::AccountProfile(const QString& id, const QString& name)
{
	m_id = id;
	m_name = name;
}

AccountProfile::AccountProfile(const AccountProfile& other)
{
	m_id = other.m_id;
	m_name = other.m_name;
}

QString AccountProfile::id() const
{
	return m_id;
}

QString AccountProfile::name() const
{
	return m_name;
}


