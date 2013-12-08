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
#include "flows/RefreshTask.h"
#include "flows/AuthenticateTask.h"

#include <QUuid>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegExp>

#include <logger/QsLog.h>

MojangAccountPtr MojangAccount::loadFromJson(const QJsonObject &object)
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

	QJsonArray profileArray = object.value("profiles").toArray();
	if (profileArray.size() < 1)
	{
		QLOG_ERROR() << "Can't load Mojang account with username \"" << username << "\". No profiles found.";
		return nullptr;
	}

	QList<AccountProfile> profiles;
	for (QJsonValue profileVal : profileArray)
	{
		QJsonObject profileObject = profileVal.toObject();
		QString id = profileObject.value("id").toString("");
		QString name = profileObject.value("name").toString("");
		if (id.isEmpty() || name.isEmpty())
		{
			QLOG_WARN() << "Unable to load a profile because it was missing an ID or a name.";
			continue;
		}
		profiles.append({id, name});
	}

	MojangAccountPtr account(new MojangAccount());
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

MojangAccountPtr MojangAccount::createFromUsername(const QString& username)
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
		profileArray.append(profileObj);
	}
	json.insert("profiles", profileArray);

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

const AccountProfile* MojangAccount::currentProfile() const
{
	if(m_currentProfile == -1)
		return nullptr;
	return &m_profiles[m_currentProfile];
}

AccountStatus MojangAccount::accountStatus() const
{
	if(m_accessToken.isEmpty())
		return NotVerified;
	if(!m_online)
		return Verified;
	return Online;
}

std::shared_ptr<Task> MojangAccount::login(QString password)
{
	if(m_currentTask)
		return m_currentTask;
	if(password.isEmpty())
	{
		m_currentTask.reset(new RefreshTask(this));
	}
	else
	{
		m_currentTask.reset(new AuthenticateTask(this, password));
	}
	connect(m_currentTask.get(), SIGNAL(succeeded()), SLOT(authSucceeded()));
	connect(m_currentTask.get(), SIGNAL(failed(QString)), SLOT(authFailed(QString)));
	return m_currentTask;
}

void MojangAccount::authSucceeded()
{
	m_online = true;
	m_currentTask.reset();
	emit changed();
}

void MojangAccount::authFailed(QString reason)
{
	// This is emitted when the yggdrasil tasks time out or are cancelled.
	// -> we treat the error as no-op
	if(reason != "Yggdrasil task cancelled.")
	{
		m_online = false;
		m_accessToken = QString();
		emit changed();
	}
	m_currentTask.reset();
}
