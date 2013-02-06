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

#ifndef LOGINRESPONSE_H
#define LOGINRESPONSE_H

#include <QObject>

class LoginResponse : public QObject
{
	Q_OBJECT
public:
	explicit LoginResponse(const QString &username, const QString &sessionID, 
						   qint64 latestVersion, QObject *parent = 0);
	LoginResponse();
	LoginResponse(const LoginResponse& other);
	
	QString getUsername() const;
	void setUsername(const QString& username);
	
	QString getSessionID() const;
	void setSessionID(const QString& sessionID);
	
	qint64 getLatestVersion() const;
	void setLatestVersion(qint64 v);
	
private:
	QString username;
	QString sessionID;
	qint64 latestVersion;
};

Q_DECLARE_METATYPE(LoginResponse)

#endif // LOGINRESPONSE_H
