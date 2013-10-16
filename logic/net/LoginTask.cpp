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

#include "LoginTask.h"
#include "MultiMC.h"
#include <settingsobject.h>

#include <QStringList>

#include <QNetworkReply>
#include <QNetworkRequest>

#include <QUrl>
#include <QUrlQuery>
#include <QJsonParseError>
#include <QJsonObject>

LoginTask::LoginTask(const UserInfo &uInfo, QObject *parent) : Task(parent), uInfo(uInfo)
{
}

void LoginTask::executeTask()
{
	yggdrasilLogin();
}

void LoginTask::legacyLogin()
{
	setStatus(tr("Logging in..."));
	auto worker = MMC->qnam();
	connect(worker.get(), SIGNAL(finished(QNetworkReply *)), this,
			SLOT(processLegacyReply(QNetworkReply *)));

	QUrl loginURL("https://login.minecraft.net/");
	QNetworkRequest netRequest(loginURL);
	netRequest.setHeader(QNetworkRequest::ContentTypeHeader,
						 "application/x-www-form-urlencoded");

	QUrlQuery params;
	params.addQueryItem("user", uInfo.username);
	params.addQueryItem("password", uInfo.password);
	params.addQueryItem("version", "13");

	netReply = worker->post(netRequest, params.query(QUrl::EncodeSpaces).toUtf8());
}

void LoginTask::parseLegacyReply(QByteArray data)
{
	QString responseStr = QString::fromUtf8(data);

	QStringList strings = responseStr.split(":");
	if (strings.count() >= 4)
	{
		// strings[1] is the download ticket. It isn't used anymore.
		QString username = strings[2];
		QString sessionID = strings[3];
		/*
		struct LoginResponse
		{
			QString username;
			QString session_id;
			QString player_name;
			QString player_id;
			QString client_id;
		};
		*/
		result = {username, sessionID, username, QString()};
		emitSucceeded();
	}
	else
	{
		if (responseStr.toLower() == "bad login")
			emitFailed(tr("Invalid username or password."));
		else if (responseStr.toLower() == "old version")
			emitFailed(tr("Launcher outdated, please update."));
		else
			emitFailed(tr("Login failed: %1").arg(responseStr));
	}
}

void LoginTask::processLegacyReply(QNetworkReply *reply)
{
	processReply(reply, &LoginTask::parseLegacyReply);
}

void LoginTask::processYggdrasilReply(QNetworkReply *reply)
{
	processReply(reply, &LoginTask::parseYggdrasilReply);
}

void LoginTask::processReply(QNetworkReply *reply, std::function<void (LoginTask*, QByteArray)> parser)
{
	if (netReply != reply)
		return;
	// Check for errors.
	switch (reply->error())
	{
	case QNetworkReply::NoError:
	{
		// Check the response code.
		int responseCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

		switch (responseCode)
		{
		case 200:
			parser(this, reply->readAll());
			break;

		default:
			emitFailed(tr("Login failed: Unknown HTTP code %1 encountered.").arg(responseCode));
			break;
		}

		break;
	}

	case QNetworkReply::OperationCanceledError:
		emitFailed(tr("Login canceled."));
		break;

	default:
		int responseCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

		switch (responseCode)
		{
		case 403:
			emitFailed(tr("Invalid username or password."));
			break;

		case 503:
			emitFailed(tr("The login servers are currently unavailable. Check "
						  "http://help.mojang.com/ for more info."));
			break;

		default:
			QLOG_DEBUG() << "Login failed with QNetworkReply code:" << reply->error();
			emitFailed(tr("Login failed: %1").arg(reply->errorString()));
			break;
		}
	}
}

void LoginTask::yggdrasilLogin()
{
	setStatus(tr("Logging in..."));
	auto worker = MMC->qnam();
	connect(worker.get(), SIGNAL(finished(QNetworkReply *)), this,
			SLOT(processYggdrasilReply(QNetworkReply *)));

	/*
	{
		// agent def. version might be incremented at some point
		"agent":{"name":"Minecraft","version":1},
		"username": "mojang account name",
		"password": "mojang account password",
		// client token is optional. but we supply one anyway
		"clientToken": "client identifier"
	}
	*/

	QUrl loginURL("https://authserver.mojang.com/authenticate");
	QNetworkRequest netRequest(loginURL);
	netRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	auto settings = MMC->settings();
	QString clientToken = settings->get("YggdrasilClientToken").toString();
	// escape the {}
	clientToken.remove('{');
	clientToken.remove('}');
	// create the request
	QString requestConstent;
	requestConstent += "{";
	requestConstent += "    \"agent\":{\"name\":\"Minecraft\",\"version\":1},\n";
	requestConstent += "    \"username\":\"" + uInfo.username + "\",\n";
	requestConstent += "    \"password\":\"" + uInfo.password + "\",\n";
	requestConstent += "    \"clientToken\":\"" + clientToken + "\"\n";
	requestConstent += "}";
	netReply = worker->post(netRequest, requestConstent.toUtf8());
}

/*
{
  "accessToken": "random access token",  // hexadecimal
  "clientToken": "client identifier",    // identical to the one received
  "availableProfiles": [                 // only present if the agent field was received
    {
      "id": "profile identifier",        // hexadecimal
      "name": "player name"
    }
  ],
  "selectedProfile": {                   // only present if the agent field was received
    "id": "profile identifier",
    "name": "player name"
  }
}
*/
void LoginTask::parseYggdrasilReply(QByteArray data)
{
	QJsonParseError jsonError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);
	if (jsonError.error != QJsonParseError::NoError)
	{
		emitFailed(tr("Login failed: %1").arg(jsonError.errorString()));
		return;
	}

	if (!jsonDoc.isObject())
	{
		emitFailed(tr("Login failed: BAD FORMAT #1"));
		return;
	}

	QJsonObject root = jsonDoc.object();

	QString accessToken = root.value("accessToken").toString();
	QString clientToken = root.value("clientToken").toString();
	QString playerID;
	QString playerName;
	auto selectedProfile = root.value("selectedProfile");
	if(selectedProfile.isObject())
	{
		auto selectedProfileO = selectedProfile.toObject();
		playerID = selectedProfileO.value("id").toString();
		playerName = selectedProfileO.value("name").toString();
	}
	QString sessionID = "token:" + accessToken + ":" + playerID;
	/*
	struct LoginResponse
	{
		QString username;
		QString session_id;
		QString player_name;
		QString player_id;
		QString client_id;
	};
	*/
	
	result = {uInfo.username, sessionID, playerName, playerID, accessToken};
	emitSucceeded();
}
