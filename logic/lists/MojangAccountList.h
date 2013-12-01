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
#include <QVariant>
#include <QAbstractListModel>
#include <QSharedPointer>

#include "logic/auth/MojangAccount.h"

/*!
 * \brief List of available Mojang accounts.
 * This should be loaded in the background by MultiMC on startup.
 *
 * This class also inherits from QAbstractListModel. Methods from that
 * class determine how this list shows up in a list view. Said methods
 * all have a default implementation, but they can be overridden by subclasses to
 * change the behavior of the list.
 */
class MojangAccountList : public QAbstractListModel
{
	Q_OBJECT
public:
	enum ModelRoles
	{
		PointerRole = 0x34B1CB48
	};

	enum VListColumns
	{
		// TODO: Add icon column.

		// First column - Active?
		ActiveColumn = 0,

		// Second column - Name
		NameColumn,
	};

	explicit MojangAccountList(QObject *parent = 0);

	//! Gets the account at the given index.
	virtual const MojangAccountPtr at(int i) const;

	//! Returns the number of accounts in the list.
	virtual int count() const;

	//////// List Model Functions ////////
	virtual QVariant data(const QModelIndex &index, int role) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual int rowCount(const QModelIndex &parent) const;
	virtual int columnCount(const QModelIndex &parent) const;

	/*!
	 * Adds a the given Mojang account to the account list.
	 */
	virtual void addAccount(const MojangAccountPtr account);

	/*!
	 * Removes the mojang account with the given username from the account list.
	 */
	virtual void removeAccount(const QString &username);

	/*!
	 * Removes the account at the given QModelIndex.
	 */
	virtual void removeAccount(QModelIndex index);

	/*!
	 * \brief Finds an account by its username.
	 * \param The username of the account to find.
	 * \return A const pointer to the account with the given username. NULL if
	 * one doesn't exist.
	 */
	virtual MojangAccountPtr findAccount(const QString &username) const;

	/*!
	 * Sets the default path to save the list file to.
	 * If autosave is true, this list will automatically save to the given path whenever it changes.
	 * THIS FUNCTION DOES NOT LOAD THE LIST. If you set autosave, be sure to call loadList() immediately
	 * after calling this function to ensure an autosaved change doesn't overwrite the list you intended
	 * to load.
	 */
	virtual void setListFilePath(QString path, bool autosave = false);

	/*!
	 * \brief Loads the account list from the given file path.
	 * If the given file is an empty string (default), will load from the default account list file.
	 * \return True if successful, otherwise false.
	 */
	virtual bool loadList(const QString &file = "");

	/*!
	 * \brief Saves the account list to the given file.
	 * If the given file is an empty string (default), will save from the default account list file.
	 * \return True if successful, otherwise false.
	 */
	virtual bool saveList(const QString &file = "");

	/*!
	 * \brief Gets a pointer to the account that the user has selected as their "active" account.
	 * Which account is active can be overridden on a per-instance basis, but this will return the one that
	 * is set as active globally.
	 * \return The currently active MojangAccount. If there isn't an active account, returns a null pointer.
	 */
	virtual MojangAccountPtr activeAccount() const;

	/*!
	 * Sets the given account as the current active account.
	 * If the username given is an empty string, sets the active account to nothing.
	 */
	virtual void setActiveAccount(const QString &username);

signals:
	/*!
	 * Signal emitted to indicate that the account list has changed.
	 * This will also fire if the value of an element in the list changes (will be implemented
	 * later).
	 */
	void listChanged();

	/*!
	 * Signal emitted to indicate that the active account has changed.
	 */
	void activeAccountChanged();

public
slots:
	/**
	 * This is called when one of the accounts changes and the list needs to be updated
	 */
	void accountChanged();

protected:
	/*!
	 * Called whenever the list changes.
	 * This emits the listChanged() signal and autosaves the list (if autosave is enabled).
	 */
	void onListChanged();

	/*!
	 * Called whenever the active account changes.
	 * Emits the activeAccountChanged() signal and autosaves the list if enabled.
	 */
	void onActiveChanged();

	QList<MojangAccountPtr> m_accounts;

	/*!
	 * Username of the account that is currently active.
	 * Empty string if no account is active.
	 */
	QString m_activeAccount;

	//! Path to the account list file. Empty string if there isn't one.
	QString m_listFilePath;

	/*!
	 * If true, the account list will automatically save to the account list path when it changes.
	 * Ignored if m_listFilePath is blank.
	 */
	bool m_autosave = false;

protected
slots:
	/*!
	 * Updates this list with the given list of accounts.
	 * This is done by copying each account in the given list and inserting it
	 * into this one.
	 * We need to do this so that we can set the parents of the accounts are set to this
	 * account list. This can't be done in the load task, because the accounts the load
	 * task creates are on the load task's thread and Qt won't allow their parents
	 * to be set to something created on another thread.
	 * To get around that problem, we invoke this method on the GUI thread, which
	 * then copies the accounts and sets their parents correctly.
	 * \param accounts List of accounts whose parents should be set.
	 */
	virtual void updateListData(QList<MojangAccountPtr> versions);
};
