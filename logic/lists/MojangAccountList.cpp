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

#include "logic/lists/MojangAccountList.h"

#include <QIODevice>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>

#include "logger/QsLog.h"

#include "logic/auth/MojangAccount.h"

#define ACCOUNT_LIST_FORMAT_VERSION 1

MojangAccountList::MojangAccountList(QObject *parent) : QAbstractListModel(parent)
{
}

MojangAccountPtr MojangAccountList::findAccount(const QString &username) const
{
	for (int i = 0; i < count(); i++)
	{
		MojangAccountPtr account = at(i);
		if (account->username() == username)
			return account;
	}
	return nullptr;
}


const MojangAccountPtr MojangAccountList::at(int i) const
{
	return MojangAccountPtr(m_accounts.at(i));
}

void MojangAccountList::addAccount(const MojangAccountPtr account)
{
	beginResetModel();
	m_accounts.append(account);
	endResetModel();
	onListChanged();
}

void MojangAccountList::removeAccount(const QString& username)
{
	beginResetModel();
	for (auto account : m_accounts)
	{
		if (account->username() == username)
		{
			m_accounts.removeOne(account);
			return;
		}
	}
	endResetModel();
	onListChanged();
}

void MojangAccountList::removeAccount(QModelIndex index)
{
	beginResetModel();
	m_accounts.removeAt(index.row());
	endResetModel();
	onListChanged();
}


MojangAccountPtr MojangAccountList::activeAccount() const
{
	if (m_activeAccount.isEmpty())
		return nullptr;
	else
		return findAccount(m_activeAccount);
}

void MojangAccountList::setActiveAccount(const QString& username)
{
	beginResetModel();
	for (MojangAccountPtr account : m_accounts)
		if (account->username() == username)
			m_activeAccount = username;
	endResetModel();
	onListChanged();
}


void MojangAccountList::onListChanged()
{
	if (m_autosave)
		// TODO: Alert the user if this fails.
		saveList();
	emit listChanged();
}


int MojangAccountList::count() const
{
	return m_accounts.count();
}


QVariant MojangAccountList::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (index.row() > count())
		return QVariant();

	MojangAccountPtr account = at(index.row());

	switch (role)
	{
	case Qt::DisplayRole:
		switch (index.column())
		{
		case ActiveColumn:
			return account->username() == m_activeAccount;

		case NameColumn:
			return account->username();

		default:
			return QVariant();
		}

	case Qt::ToolTipRole:
		return account->username();

	case PointerRole:
		return qVariantFromValue(account);

	default:
		return QVariant();
	}
}

QVariant MojangAccountList::headerData(int section, Qt::Orientation orientation, int role) const
{
	switch (role)
	{
	case Qt::DisplayRole:
		switch (section)
		{
		case ActiveColumn:
			return "Active?";

		case NameColumn:
			return "Name";

		default:
			return QVariant();
		}

	case Qt::ToolTipRole:
		switch (section)
		{
		case NameColumn:
			return "The name of the version.";

		default:
			return QVariant();
		}

	default:
		return QVariant();
	}
}

int MojangAccountList::rowCount(const QModelIndex &parent) const
{
	// Return count
	return count();
}

int MojangAccountList::columnCount(const QModelIndex &parent) const
{
	return 2;
}

void MojangAccountList::updateListData(QList<MojangAccountPtr> versions)
{
	beginResetModel();
	m_accounts = versions;
	endResetModel();
}

bool MojangAccountList::loadList(const QString& filePath)
{
	QString path = filePath;
	if (path.isEmpty()) path = m_listFilePath;
	if (path.isEmpty())
	{
		QLOG_ERROR() << "Can't load Mojang account list. No file path given and no default set.";
		return false;
	}

	QFile file(path);
	
	// Try to open the file and fail if we can't.
	// TODO: We should probably report this error to the user.
	if (!file.open(QIODevice::ReadOnly))
	{
		QLOG_ERROR() << QString("Failed to read the account list file (%1).").arg(path).toUtf8();
		return false;
	}

	// Read the file and close it.
	QByteArray jsonData = file.readAll();
	file.close();

	QJsonParseError parseError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);

	// Fail if the JSON is invalid.
	if (parseError.error != QJsonParseError::NoError)
	{
		QLOG_ERROR() << QString("Failed to parse account list file: %1 at offset %2")
							.arg(parseError.errorString(), QString::number(parseError.offset))
							.toUtf8();
		return false;
	}

	// Make sure the root is an object.
	if (!jsonDoc.isObject())
	{
		QLOG_ERROR() << "Invalid account list JSON: Root should be an array.";
		return false;
	}

	QJsonObject root = jsonDoc.object();

	// Make sure the format version matches.
	if (root.value("formatVersion").toVariant().toInt() != ACCOUNT_LIST_FORMAT_VERSION)
	{
		QString newName = "accounts-old.json";
		QLOG_WARN() << "Format version mismatch when loading account list. Existing one will be renamed to"
					<< newName;

		// Attempt to rename the old version.
		file.rename(newName);
		return false;
	}

	// Now, load the accounts array.
	beginResetModel();
	QJsonArray accounts = root.value("accounts").toArray();
	for (QJsonValue accountVal : accounts)
	{
		QJsonObject accountObj = accountVal.toObject();
		MojangAccountPtr account = MojangAccount::loadFromJson(accountObj);
		if (account.get() != nullptr)
		{
			m_accounts.append(account);
		}
		else
		{
			QLOG_WARN() << "Failed to load an account.";
		}
	}
	endResetModel();

	// Load the active account.
	m_activeAccount = root.value("activeAccount").toString("");
	
	return true;
}

bool MojangAccountList::saveList(const QString& filePath)
{
	QString path(filePath);
	if (path.isEmpty()) path = m_listFilePath;
	if (path.isEmpty())
	{
		QLOG_ERROR() << "Can't save Mojang account list. No file path given and no default set.";
		return false;
	}

	QLOG_INFO() << "Writing account list to" << path;

	QLOG_DEBUG() << "Building JSON data structure.";
	// Build the JSON document to write to the list file.
	QJsonObject root;

	root.insert("formatVersion", ACCOUNT_LIST_FORMAT_VERSION);

	// Build a list of accounts.
	QLOG_DEBUG() << "Building account array.";
	QJsonArray accounts;
	for (MojangAccountPtr account : m_accounts)
	{
		QJsonObject accountObj = account->saveToJson();
		accounts.append(accountObj);
	}

	// Insert the account list into the root object.
	root.insert("accounts", accounts);

	// Save the active account.
	root.insert("activeAccount", m_activeAccount);

	// Create a JSON document object to convert our JSON to bytes.
	QJsonDocument doc(root);


	// Now that we're done building the JSON object, we can write it to the file.
	QLOG_DEBUG() << "Writing account list to file.";
	QFile file(path);

	// Try to open the file and fail if we can't.
	// TODO: We should probably report this error to the user.
	if (!file.open(QIODevice::WriteOnly))
	{
		QLOG_ERROR() << QString("Failed to read the account list file (%1).").arg(path).toUtf8();
		return false;
	}

	// Write the JSON to the file.
	file.write(doc.toJson());
	file.close();

	QLOG_INFO() << "Saved account list to" << path;

	return true;
}

void MojangAccountList::setListFilePath(QString path, bool autosave)
{
	m_listFilePath = path;
	autosave = autosave;
}

