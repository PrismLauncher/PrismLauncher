/* Copyright 2013-2014 MultiMC Contributors
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
#include <QPair>
#include <QMap>

#include <memory>
#include "AuthSession.h"

class Task;
class YggdrasilTask;
class MojangAccount;

typedef std::shared_ptr<MojangAccount> MojangAccountPtr;
Q_DECLARE_METATYPE(MojangAccountPtr)

/**
 * A profile within someone's Mojang account.
 *
 * Currently, the profile system has not been implemented by Mojang yet,
 * but we might as well add some things for it in MultiMC right now so
 * we don't have to rip the code to pieces to add it later.
 */
struct AccountProfile
{
	QString id;
	QString name;
	bool legacy;
};

enum AccountStatus
{
	NotVerified,
	Verified
};

/**
 * Object that stores information about a certain Mojang account.
 *
 * Said information may include things such as that account's username, client token, and access
 * token if the user chose to stay logged in.
 */
class MojangAccount : public QObject
{
	Q_OBJECT
public: /* construction */
	//! Do not copy accounts. ever.
	explicit MojangAccount(const MojangAccount &other, QObject *parent) = delete;

	//! Default constructor
	explicit MojangAccount(QObject *parent = 0) : QObject(parent) {};

	//! Creates an empty account for the specified user name.
	static MojangAccountPtr createFromUsername(const QString &username);

	//! Loads a MojangAccount from the given JSON object.
	static MojangAccountPtr loadFromJson(const QJsonObject &json);

	//! Saves a MojangAccount to a JSON object and returns it.
	QJsonObject saveToJson() const;

public: /* manipulation */
		/**
	 * Sets the currently selected profile to the profile with the given ID string.
	 * If profileId is not in the list of available profiles, the function will simply return
	 * false.
	 */
	bool setCurrentProfile(const QString &profileId);

	/**
	 * Attempt to login. Empty password means we use the token.
	 * If the attempt fails because we already are performing some task, it returns false.
	 */
	std::shared_ptr<YggdrasilTask> login(AuthSessionPtr session,
										 QString password = QString());

public: /* queries */
	const QString &username() const
	{
		return m_username;
	}

	const QString &clientToken() const
	{
		return m_clientToken;
	}

	const QString &accessToken() const
	{
		return m_accessToken;
	}

	const QList<AccountProfile> &profiles() const
	{
		return m_profiles;
	}

	const User &user()
	{
		return m_user;
	}

	//! Returns the currently selected profile (if none, returns nullptr)
	const AccountProfile *currentProfile() const;

	//! Returns whether the account is NotVerified, Verified or Online
	AccountStatus accountStatus() const;

signals:
	/**
	 * This signal is emitted when the account changes
	 */
	void changed();

	// TODO: better signalling for the various possible state changes - especially errors

protected: /* variables */
	QString m_username;

	// Used to identify the client - the user can have multiple clients for the same account
	// Think: different launchers, all connecting to the same account/profile
	QString m_clientToken;

	// Blank if not logged in.
	QString m_accessToken;

	// Index of the selected profile within the list of available
	// profiles. -1 if nothing is selected.
	int m_currentProfile = -1;

	// List of available profiles.
	QList<AccountProfile> m_profiles;

	// the user structure, whatever it is.
	User m_user;

	// current task we are executing here
	std::shared_ptr<YggdrasilTask> m_currentTask;

private
slots:
	void authSucceeded();
	void authFailed(QString reason);

private:
	void fillSession(AuthSessionPtr session);

public:
	friend class YggdrasilTask;
	friend class AuthenticateTask;
	friend class ValidateTask;
	friend class RefreshTask;
};
