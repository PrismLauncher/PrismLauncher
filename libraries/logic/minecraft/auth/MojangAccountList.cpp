/* Copyright 2013-2015 MultiMC Contributors
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

#include "MojangAccountList.h"
#include "MojangAccount.h"

#include <QIODevice>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDir>

#include <QDebug>

#include <FileSystem.h>

#define ACCOUNT_LIST_FORMAT_VERSION 2

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
	connect(account.get(), SIGNAL(changed()), SLOT(accountChanged()));
	m_accounts.append(account);
	endResetModel();
	onListChanged();
}

void MojangAccountList::removeAccount(const QString &username)
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
	return m_activeAccount;
}

void MojangAccountList::setActiveAccount(const QString &username)
{
	beginResetModel();
	if (username.isEmpty())
	{
		m_activeAccount = nullptr;
	}
	else
	{
		for (MojangAccountPtr account : m_accounts)
		{
			if (account->username() == username)
				m_activeAccount = account;
		}
	}
	endResetModel();
	onActiveChanged();
}

void MojangAccountList::accountChanged()
{
	// the list changed. there is no doubt.
	onListChanged();
}

void MojangAccountList::onListChanged()
{
	if (m_autosave)
		// TODO: Alert the user if this fails.
		saveList();

	emit listChanged();
}

void MojangAccountList::onActiveChanged()
{
	if (m_autosave)
		saveList();

	emit activeAccountChanged();
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

	case Qt::CheckStateRole:
		switch (index.column())
		{
		case ActiveColumn:
			return account == m_activeAccount;
		}

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
			return tr("Active?");

		case NameColumn:
			return tr("Name");

		default:
			return QVariant();
		}

	case Qt::ToolTipRole:
		switch (section)
		{
		case NameColumn:
			return tr("The name of the version.");

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

Qt::ItemFlags MojangAccountList::flags(const QModelIndex &index) const
{
	if (index.row() < 0 || index.row() >= rowCount(index) || !index.isValid())
	{
		return Qt::NoItemFlags;
	}

	return Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool MojangAccountList::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (index.row() < 0 || index.row() >= rowCount(index) || !index.isValid())
	{
		return false;
	}

	if(role == Qt::CheckStateRole)
	{
		if(value == Qt::Checked)
		{
			MojangAccountPtr account = this->at(index.row());
			this->setActiveAccount(account->username());
		}
	}

	emit dataChanged(index, index);
	return true;
}

void MojangAccountList::updateListData(QList<MojangAccountPtr> versions)
{
	beginResetModel();
	m_accounts = versions;
	endResetModel();
}

bool MojangAccountList::loadList(const QString &filePath)
{
	QString path = filePath;
	if (path.isEmpty())
		path = m_listFilePath;
	if (path.isEmpty())
	{
		qCritical() << "Can't load Mojang account list. No file path given and no default set.";
		return false;
	}

	QFile file(path);

	// Try to open the file and fail if we can't.
	// TODO: We should probably report this error to the user.
	if (!file.open(QIODevice::ReadOnly))
	{
		qCritical() << QString("Failed to read the account list file (%1).").arg(path).toUtf8();
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
		qCritical() << QString("Failed to parse account list file: %1 at offset %2")
							.arg(parseError.errorString(), QString::number(parseError.offset))
							.toUtf8();
		return false;
	}

	// Make sure the root is an object.
	if (!jsonDoc.isObject())
	{
		qCritical() << "Invalid account list JSON: Root should be an array.";
		return false;
	}

	QJsonObject root = jsonDoc.object();

	// Make sure the format version matches.
	if (root.value("formatVersion").toVariant().toInt() != ACCOUNT_LIST_FORMAT_VERSION)
	{
		QString newName = "accounts-old.json";
		qWarning() << "Format version mismatch when loading account list. Existing one will be renamed to"
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
			connect(account.get(), SIGNAL(changed()), SLOT(accountChanged()));
			m_accounts.append(account);
		}
		else
		{
			qWarning() << "Failed to load an account.";
		}
	}
	// Load the active account.
	m_activeAccount = findAccount(root.value("activeAccount").toString(""));
	endResetModel();
	return true;
}

bool MojangAccountList::saveList(const QString &filePath)
{
	QString path(filePath);
	if (path.isEmpty())
		path = m_listFilePath;
	if (path.isEmpty())
	{
		qCritical() << "Can't save Mojang account list. No file path given and no default set.";
		return false;
	}

	// make sure the parent folder exists
	if(!FS::ensureFilePathExists(path))
		return false;

	// make sure the file wasn't overwritten with a folder before (fixes a bug)
	QFileInfo finfo(path);
	if(finfo.isDir())
	{
		QDir badDir(path);
		badDir.removeRecursively();
	}

	qDebug() << "Writing account list to" << path;

	qDebug() << "Building JSON data structure.";
	// Build the JSON document to write to the list file.
	QJsonObject root;

	root.insert("formatVersion", ACCOUNT_LIST_FORMAT_VERSION);

	// Build a list of accounts.
	qDebug() << "Building account array.";
	QJsonArray accounts;
	for (MojangAccountPtr account : m_accounts)
	{
		QJsonObject accountObj = account->saveToJson();
		accounts.append(accountObj);
	}

	// Insert the account list into the root object.
	root.insert("accounts", accounts);

	if(m_activeAccount)
	{
		// Save the active account.
		root.insert("activeAccount", m_activeAccount->username());
	}

	// Create a JSON document object to convert our JSON to bytes.
	QJsonDocument doc(root);

	// Now that we're done building the JSON object, we can write it to the file.
	qDebug() << "Writing account list to file.";
	QFile file(path);

	// Try to open the file and fail if we can't.
	// TODO: We should probably report this error to the user.
	if (!file.open(QIODevice::WriteOnly))
	{
		qCritical() << QString("Failed to read the account list file (%1).").arg(path).toUtf8();
		return false;
	}

	// Write the JSON to the file.
	file.write(doc.toJson());
	file.setPermissions(QFile::ReadOwner|QFile::WriteOwner|QFile::ReadUser|QFile::WriteUser);
	file.close();

	qDebug() << "Saved account list to" << path;

	return true;
}

void MojangAccountList::setListFilePath(QString path, bool autosave)
{
	m_listFilePath = path;
	m_autosave = autosave;
}

bool MojangAccountList::anyAccountIsValid()
{
	for(auto account:m_accounts)
	{
		if(account->accountStatus() != NotVerified)
			return true;
	}
	return false;
}
