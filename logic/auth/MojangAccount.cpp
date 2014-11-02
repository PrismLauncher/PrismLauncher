/* Copyright 2013-2014 MultiMC Contributors
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
#include "flows/RefreshTask.h"
#include "flows/AuthenticateTask.h"

#include <QUuid>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegExp>
#include <QStringList>
#include <QJsonDocument>

#include <logger/QsLog.h>

MojangAccountPtr MojangAccount::loadFromJson(const QJsonObject &object)
{
	// The JSON object must at least have a username for it to be valid.
	if (!object.value("username").isString())
	{
		QLOG_ERROR() << "Can't load Mojang account info from JSON object. Username field is "
						"missing or of the wrong type.";
		return nullptr;
	}

	QString username = object.value("username").toString("");
	QString clientToken = object.value("clientToken").toString("");
	QString accessToken = object.value("accessToken").toString("");

	QJsonArray profileArray = object.value("profiles").toArray();
	if (profileArray.size() < 1)
	{
		QLOG_ERROR() << "Can't load Mojang account with username \"" << username
					 << "\". No profiles found.";
		return nullptr;
	}

	QList<AccountProfile> profiles;
	for (QJsonValue profileVal : profileArray)
	{
		QJsonObject profileObject = profileVal.toObject();
		QString id = profileObject.value("id").toString("");
		QString name = profileObject.value("name").toString("");
		bool legacy = profileObject.value("legacy").toBool(false);
		if (id.isEmpty() || name.isEmpty())
		{
			QLOG_WARN() << "Unable to load a profile because it was missing an ID or a name.";
			continue;
		}
		profiles.append({id, name, legacy});
	}

	MojangAccountPtr account(new MojangAccount());
	if (object.value("user").isObject())
	{
		User u;
		QJsonObject userStructure = object.value("user").toObject();
		u.id = userStructure.value("id").toString();
		/*
		QJsonObject propMap = userStructure.value("properties").toObject();
		for(auto key: propMap.keys())
		{
			auto values = propMap.operator[](key).toArray();
			for(auto value: values)
				u.properties.insert(key, value.toString());
		}
		*/
		account->m_user = u;
	}
	account->m_username = username;
	account->m_clientToken = clientToken;
	account->m_accessToken = accessToken;
	account->m_profiles = profiles;

	// Get the currently selected profile.
	QString currentProfile = object.value("activeProfile").toString("");
	if (!currentProfile.isEmpty())
		account->setCurrentProfile(currentProfile);

	return account;
}

MojangAccountPtr MojangAccount::createFromUsername(const QString &username)
{
	MojangAccountPtr account(new MojangAccount());
	account->m_clientToken = QUuid::createUuid().toString().remove(QRegExp("[{}-]"));
	account->m_username = username;
	return account;
}

QJsonObject MojangAccount::saveToJson() const
{
	QJsonObject json;
	json.insert("username", m_username);
	json.insert("clientToken", m_clientToken);
	json.insert("accessToken", m_accessToken);

	QJsonArray profileArray;
	for (AccountProfile profile : m_profiles)
	{
		QJsonObject profileObj;
		profileObj.insert("id", profile.id);
		profileObj.insert("name", profile.name);
		profileObj.insert("legacy", profile.legacy);
		profileArray.append(profileObj);
	}
	json.insert("profiles", profileArray);

	QJsonObject userStructure;
	{
		userStructure.insert("id", m_user.id);
		/*
		QJsonObject userAttrs;
		for(auto key: m_user.properties.keys())
		{
			auto array = QJsonArray::fromStringList(m_user.properties.values(key));
			userAttrs.insert(key, array);
		}
		userStructure.insert("properties", userAttrs);
		*/
	}
	json.insert("user", userStructure);

	if (m_currentProfile != -1)
		json.insert("activeProfile", currentProfile()->id);

	return json;
}

bool MojangAccount::setCurrentProfile(const QString &profileId)
{
	for (int i = 0; i < m_profiles.length(); i++)
	{
		if (m_profiles[i].id == profileId)
		{
			m_currentProfile = i;
			return true;
		}
	}
	return false;
}

const AccountProfile *MojangAccount::currentProfile() const
{
	if (m_currentProfile == -1)
		return nullptr;
	return &m_profiles[m_currentProfile];
}

AccountStatus MojangAccount::accountStatus() const
{
	if (m_accessToken.isEmpty())
		return NotVerified;
	else
		return Verified;
}

std::shared_ptr<YggdrasilTask> MojangAccount::login(AuthSessionPtr session,
													QString password)
{
	Q_ASSERT(m_currentTask.get() == nullptr);

	// take care of the true offline status
	if (accountStatus() == NotVerified && password.isEmpty())
	{
		if (session)
		{
			session->status = AuthSession::RequiresPassword;
			fillSession(session);
		}
		return nullptr;
	}

	if (password.isEmpty())
	{
		m_currentTask.reset(new RefreshTask(this));
	}
	else
	{
		m_currentTask.reset(new AuthenticateTask(this, password));
	}
	m_currentTask->assignSession(session);

	connect(m_currentTask.get(), SIGNAL(succeeded()), SLOT(authSucceeded()));
	connect(m_currentTask.get(), SIGNAL(failed(QString)), SLOT(authFailed(QString)));
	return m_currentTask;
}

void MojangAccount::authSucceeded()
{
	auto session = m_currentTask->getAssignedSession();
	if (session)
	{
		session->status =
			session->wants_online ? AuthSession::PlayableOnline : AuthSession::PlayableOffline;
		fillSession(session);
		session->auth_server_online = true;
	}
	m_currentTask.reset();
	emit changed();
}

void MojangAccount::authFailed(QString reason)
{
	auto session = m_currentTask->getAssignedSession();
	// This is emitted when the yggdrasil tasks time out or are cancelled.
	// -> we treat the error as no-op
	if (m_currentTask->state() == YggdrasilTask::STATE_FAILED_SOFT)
	{
		if (session)
		{
			session->status = accountStatus() == Verified ? AuthSession::PlayableOffline
														  : AuthSession::RequiresPassword;
			session->auth_server_online = false;
			fillSession(session);
		}
	}
	else
	{
		m_accessToken = QString();
		emit changed();
		if (session)
		{
			session->status = AuthSession::RequiresPassword;
			session->auth_server_online = true;
			fillSession(session);
		}
	}
	m_currentTask.reset();
}

void MojangAccount::fillSession(AuthSessionPtr session)
{
	// the user name. you have to have an user name
	session->username = m_username;
	// volatile auth token
	session->access_token = m_accessToken;
	// the semi-permanent client token
	session->client_token = m_clientToken;
	if (currentProfile())
	{
		// profile name
		session->player_name = currentProfile()->name;
		// profile ID
		session->uuid = currentProfile()->id;
		// 'legacy' or 'mojang', depending on account type
		session->user_type = currentProfile()->legacy ? "legacy" : "mojang";
		if (!session->access_token.isEmpty())
		{
			session->session = "token:" + m_accessToken + ":" + m_profiles[m_currentProfile].id;
		}
		else
		{
			session->session = "-";
		}
	}
	else
	{
		session->player_name = "Player";
		session->session = "-";
	}
	session->u = user();
}
