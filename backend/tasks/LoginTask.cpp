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

#include <QStringList>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include <QUrl>
#include <QUrlQuery>

LoginTask::LoginTask( const UserInfo& uInfo, QObject* parent ) :
	Task(parent), uInfo(uInfo)
{
	
}

void LoginTask::executeTask()
{
	setStatus("Logging in...");
	
	QNetworkAccessManager netMgr;
	connect(&netMgr, SIGNAL(finished(QNetworkReply*)),
			SLOT(processNetReply(QNetworkReply*)));
	
	QUrl loginURL("https://login.minecraft.net/");
	QNetworkRequest netRequest(loginURL);
	netRequest.setHeader(QNetworkRequest::ContentTypeHeader, 
						 "application/x-www-form-urlencoded");
	
	QUrlQuery params;
	params.addQueryItem("user", uInfo.username());
	params.addQueryItem("password", uInfo.password());
	params.addQueryItem("version", "13");
	
	netReply = netMgr.post(netRequest, params.query(QUrl::EncodeSpaces).toUtf8());
	exec();
}

void LoginTask::processNetReply(QNetworkReply *reply)
{
	// Check for errors.
	switch (reply->error())
	{
	case QNetworkReply::NoError:
	{
		// Check the response code.
		int responseCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
		
		if (responseCode == 200)
		{
			QString responseStr(reply->readAll());
			
			QStringList strings = responseStr.split(":");
			if (strings.count() >= 4)
			{
				bool parseSuccess;
				qint64 latestVersion = strings[0].toLongLong(&parseSuccess);
				if (parseSuccess)
				{
					// strings[1] is the download ticket. It isn't used anymore.
					QString username = strings[2];
					QString sessionID = strings[3];
					
					LoginResponse response{username, sessionID, latestVersion};
					emit loginComplete(response);
				}
				else
				{
					emit loginFailed("Failed to parse Minecraft version string.");
				}
			}
			else
			{
				if (responseStr.toLower() == "bad login")
					emit loginFailed("Invalid username or password.");
				else if (responseStr.toLower() == "old version")
					emit loginFailed("Launcher outdated, please update.");
				else
					emit loginFailed("Login failed: " + responseStr);
			}
		}
		else if (responseCode == 503)
		{
			emit loginFailed("The login servers are currently unavailable. "
							 "Check http://help.mojang.com/ for more info.");
		}
		else
		{
			emit loginFailed(QString("Login failed: Unknown HTTP error %1 occurred.").
							 arg(QString::number(responseCode)));
		}
		break;
	}
		
	case QNetworkReply::OperationCanceledError:
		emit loginFailed("Login canceled.");
		break;
		
	default:
		emit loginFailed("Login failed: " + reply->errorString());
		break;
	}
	
	quit();
}
