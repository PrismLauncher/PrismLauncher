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

#include <QObject>
#include <QString>
#include <QList>
#include <QJsonObject>

#include <memory>

class MojangAccount;

typedef std::shared_ptr<MojangAccount> MojangAccountPtr;
Q_DECLARE_METATYPE(MojangAccountPtr)


/**
 * Class that represents a profile within someone's Mojang account.
 *
 * Currently, the profile system has not been implemented by Mojang yet,
 * but we might as well add some things for it in MultiMC right now so
 * we don't have to rip the code to pieces to add it later.
 */
class AccountProfile
{
public:
	AccountProfile(const QString& id, const QString& name);
	AccountProfile(const AccountProfile& other);

	QString id() const;
	QString name() const;
protected:
	QString m_id;
	QString m_name;
};


typedef QList<AccountProfile> ProfileList;


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
	 * Constructs a new MojangAccount matching the given account.
	 */
	MojangAccount(const MojangAccount& other, QObject* parent);

	/**
	 * Loads a MojangAccount from the given JSON object.
	 */
	static MojangAccountPtr loadFromJson(const QJsonObject& json);

	/**
	 * Saves a MojangAccount to a JSON object and returns it.
	 */
	QJsonObject saveToJson();


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
	 * Sets the MojangAccount's client token to the given value.
	 */
	void setClientToken(const QString& token);

	/**
	 * This MojangAccount's access token.
	 * If the user has not chosen to stay logged in, this will be an empty string.
	 */
	QString accessToken() const;

	/**
	 * Changes this MojangAccount's access token to the given value.
	 */
	void setAccessToken(const QString& token);

	/**
	 * Get full session ID
	 */
	QString sessionId() const;

	/**
	 * Returns a list of the available account profiles.
	 */
	const ProfileList profiles() const;

	/**
	 * Returns a pointer to the currently selected profile.
	 * If no profile is selected, returns the first profile in the profile list or nullptr if there are none.
	 */
	const AccountProfile* currentProfile() const;

	/**
	 * Sets the currently selected profile to the profile with the given ID string.
	 * If profileId is not in the list of available profiles, the function will simply return false.
	 */
	bool setProfile(const QString& profileId);

	/**
	 * Clears the current account profile list and replaces it with the given profile list.
	 */
	void loadProfiles(const ProfileList& profiles);


protected:
	QString m_username;
	QString m_clientToken;
	QString m_accessToken; // Blank if not logged in.
	int m_currentProfile; // Index of the selected profile within the list of available profiles. -1 if nothing is selected.
	ProfileList m_profiles; // List of available profiles.
};

