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

#include <logic/auth/flows/RefreshTask.h>

#include <logic/auth/MojangAccount.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>
#include <QDebug>

#include "logger/QsLog.h"

RefreshTask::RefreshTask(MojangAccount *account, QObject *parent)
	: YggdrasilTask(account, parent)
{
}

QJsonObject RefreshTask::getRequestContent() const
{
	/*
	 * {
	 *  "clientToken": "client identifier"
	 *  "accessToken": "current access token to be refreshed"
	 *  "selectedProfile":                      // specifying this causes errors
	 *  {
	 *   "id": "profile ID"
	 *   "name": "profile name"
	 *  }
	 *  "requestUser": true/false               // request the user structure
	 * }
	 */
	QJsonObject req;
	req.insert("clientToken", m_account->m_clientToken);
	req.insert("accessToken", m_account->m_accessToken);
	/*
	{
		auto currentProfile = m_account->currentProfile();
		QJsonObject profile;
		profile.insert("id", currentProfile->id());
		profile.insert("name", currentProfile->name());
		req.insert("selectedProfile", profile);
	}
	*/
	req.insert("requestUser", true);

	return req;
}

bool RefreshTask::processResponse(QJsonObject responseData)
{
	// Read the response data. We need to get the client token, access token, and the selected
	// profile.
	QLOG_DEBUG() << "Processing authentication response.";

	// QLOG_DEBUG() << responseData;
	// If we already have a client token, make sure the one the server gave us matches our
	// existing one.
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
		QLOG_ERROR() << "Server changed our client token to '" << clientToken
					 << "'. This shouldn't happen, but it isn't really a big deal.";
		return false;
	}

	// Now, we set the access token.
	QLOG_DEBUG() << "Getting new access token.";
	QString accessToken = responseData.value("accessToken").toString("");
	if (accessToken.isEmpty())
	{
		// Fail if the server didn't give us an access token.
		// TODO: Set an error properly to display to the user.
		QLOG_ERROR() << "Server didn't send an access token.";
		return false;
	}

	// we validate that the server responded right. (our current profile = returned current
	// profile)
	QJsonObject currentProfile = responseData.value("selectedProfile").toObject();
	QString currentProfileId = currentProfile.value("id").toString("");
	if (m_account->currentProfile()->id != currentProfileId)
	{
		// TODO: Set an error to display to the user.
		QLOG_ERROR() << "Server didn't specify the same selected profile as ours.";
		return false;
	}

	// this is what the vanilla launcher passes to the userProperties launch param
	if (responseData.contains("user"))
	{
		User u;
		auto obj = responseData.value("user").toObject();
		u.id = obj.value("id").toString();
		auto propArray = obj.value("properties").toArray();
		for (auto prop : propArray)
		{
			auto propTuple = prop.toObject();
			auto name = propTuple.value("name").toString();
			auto value = propTuple.value("value").toString();
			u.properties.insert(name, value);
		}
		m_account->m_user = u;
	}


	// We've made it through the minefield of possible errors. Return true to indicate that
	// we've succeeded.
	QLOG_DEBUG() << "Finished reading refresh response.";
	// Reset the access token.
	m_account->m_accessToken = accessToken;
	return true;
}

QString RefreshTask::getEndpoint() const
{
	return "refresh";
}

QString RefreshTask::getStateMessage(const YggdrasilTask::State state) const
{
	switch (state)
	{
	case STATE_SENDING_REQUEST:
		return tr("Refreshing login token.");
	case STATE_PROCESSING_RESPONSE:
		return tr("Refreshing login token: Processing response.");
	default:
		return YggdrasilTask::getStateMessage(state);
	}
}
