
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

#include <logic/auth/AuthenticateTask.h>

#include <logic/auth/MojangAccount.h>
#include <QJsonDocument>
#include <QVariant>
#include <QDebug>

AuthenticateTask::AuthenticateTask(MojangAccount* account, const QString& password, QObject* parent) :
	YggdrasilTask(account, parent), m_password(password)
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
	 * }
	 */
	QJsonObject req;

	{
		QJsonObject agent;
		// C++ makes string literals void* for some stupid reason, so we have to tell it QString... Thanks Obama.
		agent.insert("name", QString("Minecraft"));
		agent.insert("version", 1);
		req.insert("agent", agent);
	}

	req.insert("username", getMojangAccount()->username());
	req.insert("password", m_password);
	req.insert("clientToken", getMojangAccount()->clientToken());

	return req;
}

bool AuthenticateTask::processResponse(QJsonObject responseData)
{
	qDebug() << QJsonDocument(responseData).toVariant().toString();
	return false;
}

QString AuthenticateTask::getEndpoint() const
{
	return "authenticate";
}

