/* Copyright 2013 Andrew Okin
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

#include <QObject>
#include <QString>

/**
 * Object that stores information about a certain Mojang account.
 *
 * Said information may include things such as that account's username, client token, and access 
 * token if the user chose to stay logged in.
 */
class MojangAccount : public QObject
{
Q_OBJECT
public:
	/**
	 * Constructs a new MojangAccount with the given username.
	 * The client token will be generated automatically and the access token will be blank.
	 */
	explicit MojangAccount(const QString& username, QObject* parent = 0);

	/**
	 * Constructs a new MojangAccount with the given username, client token, and access token.
	 */
	explicit MojangAccount(const QString& username, const QString& clientToken, const QString& accessToken, QObject* parent = 0);


	/**
	 * This MojangAccount's username. May be an email address if the account is migrated.
	 */
	QString username() const;

	/**
	 * This MojangAccount's client token. This is a UUID used by Mojang's auth servers to identify this client.
	 * This is unique for each MojangAccount.
	 */
	QString clientToken() const;

	/**
	 * This MojangAccount's access token.
	 * If the user has not chosen to stay logged in, this will be an empty string.
	 */
	QString accessToken() const;
	
	/**
	 * Changes this MojangAccount's access token to the given value.
	 */
	QString setAccessToken(const QString& token);

protected:
	QString m_username;
	QString m_clientToken;
	QString m_accessToken; // Blank if not logged in.
};

