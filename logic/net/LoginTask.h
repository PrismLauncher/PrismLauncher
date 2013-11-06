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

#pragma once

#include "logic/tasks/Task.h"
#include <QSharedPointer>

struct UserInfo
{
	QString username;
	QString password;
};

struct LoginResponse
{
	QString username;
	QString session_id; // session id is a combination of player id and the access token
	QString player_name;
	QString player_id;
	QString access_token;
};

class QNetworkReply;

class LoginTask : public Task
{
	Q_OBJECT
public:
	explicit LoginTask(const UserInfo &uInfo, QObject *parent = 0);
	LoginResponse getResult()
	{
		return result;
	}

protected
slots:
	void legacyLogin();
	void processLegacyReply(QNetworkReply *reply);
	void parseLegacyReply(QByteArray data);
	QString parseLegacyError(QNetworkReply *reply);

	void yggdrasilLogin();
	void processYggdrasilReply(QNetworkReply *reply);
	void parseYggdrasilReply(QByteArray data);
	QString parseYggdrasilError(QNetworkReply *reply);

	void processReply(QNetworkReply *reply, std::function<void(LoginTask *, QByteArray)>,
					  std::function<QString(LoginTask *, QNetworkReply *)>);

protected:
	void executeTask();

	LoginResponse result;
	QNetworkReply *netReply;
	UserInfo uInfo;
};
