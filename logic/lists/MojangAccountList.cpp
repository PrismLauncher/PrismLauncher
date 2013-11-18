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
#include "logic/auth/MojangAccount.h"

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
