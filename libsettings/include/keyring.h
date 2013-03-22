/* Copyright 2013 MultiMC Contributors
 *
 * Authors: Orochimarufan <orochimarufan.x3@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef KEYRING_H
#define KEYRING_H

#include <QString>

#include "libsettings_config.h"

/**
 * @file libsettings/include/keyring.h
 * Access to System Keyrings
 */

/**
 * @brief The Keyring class
 * the System Keyring/Keychain/Wallet/Vault/etc
 */
class LIBSETTINGS_EXPORT Keyring
{
public:
	/**
	 * @brief the System Keyring instance
	 * @return the Keyring instance
	 */
	static Keyring *instance();

	/**
	 * @brief store a password in the Keyring
	 * @param service the service name
	 * @param username the account name
	 * @param password the password to store
	 * @return success
	 */
	virtual bool storePassword(QString service, QString username, QString password) = 0;

	/**
	 * @brief get a password from the Keyring
	 * @param service the service name
	 * @param username the account name
	 * @return the password (success=!isNull())
	 */
	virtual QString getPassword(QString service, QString username) = 0;

	/**
	 * @brief lookup a password
	 * @param service the service name
	 * @param username the account name
	 * @return wether the password is available
	 */
	virtual bool hasPassword(QString service, QString username) = 0;

	/**
	 * @brief get a list of all stored accounts.
	 * @param service the service name
	 * @return
	 */
	virtual QStringList getStoredAccounts(QString service) = 0;

protected:
	/// fall back to StubKeyring if false
	virtual bool isValid() { return false; }

private:
	static Keyring *m_instance;
	static void destroy();
};

#endif // KEYRING_H
