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

#define DEFAULT_ACCOUNT_LIST_FILE "accounts.json"

#define ACCOUNT_LIST_FORMAT_VERSION 1

MojangAccountList::MojangAccountList(QObject *parent) : QAbstractListModel(parent)
{
}

MojangAccountPtr MojangAccountList::findAccount(const QString &username)
{
	for (int i = 0; i < count(); i++)
	{
		MojangAccountPtr account = at(i);
		if (account->username() == username)
			return account;
	}
	return MojangAccountPtr();
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
	return 1;
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
	if (path.isEmpty()) path = DEFAULT_ACCOUNT_LIST_FILE;

	QFile file(path);
	
	// Try to open the file and fail if we can't.
	// TODO: We should probably report this error to the user.
	if (!file.open(QIODevice::ReadOnly))
	{
		QLOG_ERROR() << "Failed to read the account list file (" << path << ").";
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
		QString newName = "accountlist-old.json";
		QLOG_WARN() << "Format version mismatch when loading account list. Existing one will be renamed to \""
					<< newName << "\".";

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
	
	return true;
}

