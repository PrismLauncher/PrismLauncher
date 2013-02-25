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

/*!
 * \brief The LoginResponse class represents a response received from Minecraft's login servers.
 */
class LoginResponse : public QObject
{
	Q_OBJECT
public:
	/*!
	 * \brief Creates a new instance of the LoginResponse class.
	 * \param username The user's username.
	 * \param sessionID The user's session ID.
	 * \param latestVersion The latest version of Minecraft.
	 * \param parent The parent object.
	 */
	explicit LoginResponse(const QString &username, const QString &sessionID, 
						   qint64 latestVersion, QObject *parent = 0);
	LoginResponse();
	LoginResponse(const LoginResponse& other);
	
	/*!
	 * \brief Gets the username.
	 * This one should go without saying.
	 * \return The username.
	 * \sa setUsername()
	 */
	QString username() const;
	
	/*!
	 * \brief setUsername Sets the username.
	 * \param username The new username.
	 * \sa username()
	 */
	void setUsername(const QString& username);
	
	
	/*!
	 * \brief Gets the session ID.
	 * \return The session ID.
	 * \sa setSessionID()
	 */
	QString sessionID() const;
	
	/*!
	 * \brief Sets the session ID.
	 * \param sessionID The new session ID.
	 * \sa sessionID()
	 */
	void setSessionID(const QString& sessionID);
	
	
	/*!
	 * \brief Gets the latest version.
	 * This is a value returned by the login servers when a user logs in.
	 * \return The latest version.
	 * \sa setLatestVersion()
	 */
	qint64 latestVersion() const;
	
	/*!
	 * \brief Sets the latest version.
	 * \param v The new latest version.
	 * \sa latestVersion()
	 */
	void setLatestVersion(qint64 v);
	
private:
	QString m_username;
	QString m_sessionID;
	qint64 m_latestVersion;
};

Q_DECLARE_METATYPE(LoginResponse)

#endif // LOGINRESPONSE_H
