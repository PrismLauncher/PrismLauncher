
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

#include <logic/auth/flows/AuthenticateTask.h>

#include <logic/auth/MojangAccount.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>
#include <QDebug>

#include "logger/QsLog.h"

AuthenticateTask::AuthenticateTask(MojangAccount * account, const QString &password,
								   QObject *parent)
	: YggdrasilTask(account, parent), m_password(password)
{
}

QJsonObject AuthenticateTask::getRequestContent() const
{
	/*
	 * {
	 *   "agent": {								// optional
	 *   "name": "Minecraft",					// So far this is the only encountered value
	 *   "version": 1							// This number might be increased
	 * 											// by the vanilla client in the future
	 *   },
	 *   "username": "mojang account name",		// Can be an email address or player name for
												// unmigrated accounts
	 *  "password": "mojang account password",
	 *  "clientToken": "client identifier"		// optional
	 *  "requestUser": true/false               // request the user structure
	 * }
	 */
	QJsonObject req;

	{
		QJsonObject agent;
		// C++ makes string literals void* for some stupid reason, so we have to tell it
		// QString... Thanks Obama.
		agent.insert("name", QString("Minecraft"));
		agent.insert("version", 1);
		req.insert("agent", agent);
	}

	req.insert("username", m_account->username());
	req.insert("password", m_password);
	req.insert("requestUser", true);

	// If we already have a client token, give it to the server.
	// Otherwise, let the server give us one.
	if (!m_account->m_clientToken.isEmpty())
		req.insert("clientToken", m_account->m_clientToken);

	return req;
}

bool AuthenticateTask::processResponse(QJsonObject responseData)
{
	// Read the response data. We need to get the client token, access token, and the selected
	// profile.
	QLOG_DEBUG() << "Processing authentication response.";
	// QLOG_DEBUG() << responseData;
	// If we already have a client token, make sure the one the server gave us matches our
	// existing one.
	QLOG_DEBUG() << "Getting client token.";
	QString clientToken = responseData.value("clientToken").toString("");
	if (clientToken.isEmpty())
	{
		// Fail if the server gave us an empty client token
		// TODO: Set an error properly to display to the user.
		QLOG_ERROR() << "Server didn't send a client token.";
		return false;
	}
	if (!m_account->m_clientToken.isEmpty() && clientToken != m_account->m_clientToken)
	{
		// The server changed our client token! Obey its wishes, but complain. That's what I do
		// for my parents, so...
		QLOG_WARN() << "Server changed our client token to '" << clientToken
					<< "'. This shouldn't happen, but it isn't really a big deal.";
	}
	// Set the client token.
	m_account->m_clientToken = clientToken;

	// Now, we set the access token.
	QLOG_DEBUG() << "Getting access token.";
	QString accessToken = responseData.value("accessToken").toString("");
	if (accessToken.isEmpty())
	{
		// Fail if the server didn't give us an access token.
		// TODO: Set an error properly to display to the user.
		QLOG_ERROR() << "Server didn't send an access token.";
	}
	// Set the access token.
	m_account->m_accessToken = accessToken;

	// Now we load the list of available profiles.
	// Mojang hasn't yet implemented the profile system,
	// but we might as well support what's there so we
	// don't have trouble implementing it later.
	QLOG_DEBUG() << "Loading profile list.";
	QJsonArray availableProfiles = responseData.value("availableProfiles").toArray();
	QList<AccountProfile> loadedProfiles;
	for (auto iter : availableProfiles)
	{
		QJsonObject profile = iter.toObject();
		// Profiles are easy, we just need their ID and name.
		QString id = profile.value("id").toString("");
		QString name = profile.value("name").toString("");
		bool legacy = profile.value("legacy").toBool(false);

		if (id.isEmpty() || name.isEmpty())
		{
			// This should never happen, but we might as well
			// warn about it if it does so we can debug it easily.
			// You never know when Mojang might do something truly derpy.
			QLOG_WARN() << "Found entry in available profiles list with missing ID or name "
						   "field. Ignoring it.";
		}

		// Now, add a new AccountProfile entry to the list.
		loadedProfiles.append({id, name, legacy});
	}
	// Put the list of profiles we loaded into the MojangAccount object.
	m_account->m_profiles = loadedProfiles;

	// Finally, we set the current profile to the correct value. This is pretty simple.
	// We do need to make sure that the current profile that the server gave us
	// is actually in the available profiles list.
	// If it isn't, we'll just fail horribly (*shouldn't* ever happen, but you never know).
	QLOG_DEBUG() << "Setting current profile.";
	QJsonObject currentProfile = responseData.value("selectedProfile").toObject();
	QString currentProfileId = currentProfile.value("id").toString("");
	if (currentProfileId.isEmpty())
	{
		// TODO: Set an error to display to the user.
		QLOG_ERROR() << "Server didn't specify a currently selected profile.";
		return false;
	}
	if (!m_account->setCurrentProfile(currentProfileId))
	{
		// TODO: Set an error to display to the user.
		QLOG_ERROR() << "Server specified a selected profile that wasn't in the available "
						"profiles list.";
		return false;
	}

	// this is what the vanilla launcher passes to the userProperties launch param
	// doesn't seem to be used for anything so far? I don't get any of this data on my account
	// (peterixxx)
	// is it a good idea to log this?
	if (responseData.contains("user"))
	{
		User u;
		auto obj = responseData.value("user").toObject();
		u.id = obj.value("id").toString();
		QLOG_DEBUG() << "User ID: " << u.id ;
		auto propArray = obj.value("properties").toArray();
		QLOG_DEBUG() << "User Properties: ";
		for (auto prop : propArray)
		{
			auto propTuple = prop.toObject();
			auto name = propTuple.value("name").toString();
			auto value = propTuple.value("value").toString();
			QLOG_DEBUG() << name << " : " << value;
			u.properties.insert(name, value);
		}
		m_account->m_user = u;
	}

	// We've made it through the minefield of possible errors. Return true to indicate that
	// we've succeeded.
	QLOG_DEBUG() << "Finished reading authentication response.";
	return true;
}

QString AuthenticateTask::getEndpoint() const
{
	return "authenticate";
}

QString AuthenticateTask::getStateMessage(const YggdrasilTask::State state) const
{
	switch (state)
	{
	case STATE_SENDING_REQUEST:
		return tr("Authenticating: Sending request.");
	case STATE_PROCESSING_RESPONSE:
		return tr("Authenticating: Processing response.");
	default:
		return YggdrasilTask::getStateMessage(state);
	}
}
