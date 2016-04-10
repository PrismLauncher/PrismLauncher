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

#include "BaseVersionList.h"
#include "BaseVersion.h"

BaseVersionList::BaseVersionList(QObject *parent) : QAbstractListModel(parent)
{
}

BaseVersionPtr BaseVersionList::findVersion(const QString &descriptor)
{
	for (int i = 0; i < count(); i++)
	{
		if (at(i)->descriptor() == descriptor)
			return at(i);
	}
	return BaseVersionPtr();
}

BaseVersionPtr BaseVersionList::getLatestStable() const
{
	if (count() <= 0)
		return BaseVersionPtr();
	else
		return at(0);
}

BaseVersionPtr BaseVersionList::getRecommended() const
{
	return getLatestStable();
}

QVariant BaseVersionList::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (index.row() > count())
		return QVariant();

	BaseVersionPtr version = at(index.row());

	switch (role)
	{
	case VersionPointerRole:
		return qVariantFromValue(version);

	case VersionRole:
		return version->name();

	case VersionIdRole:
		return version->descriptor();

	case TypeRole:
		return version->typeString();

	default:
		return QVariant();
	}
}

BaseVersionList::RoleList BaseVersionList::providesRoles() const
{
	return {VersionPointerRole, VersionRole, VersionIdRole, TypeRole};
}

int BaseVersionList::rowCount(const QModelIndex &parent) const
{
	// Return count
	return count();
}

int BaseVersionList::columnCount(const QModelIndex &parent) const
{
	return 1;
}

QHash<int, QByteArray> BaseVersionList::roleNames() const
{
	QHash<int, QByteArray> roles = QAbstractListModel::roleNames();
	roles.insert(VersionRole, "version");
	roles.insert(VersionIdRole, "versionId");
	roles.insert(ParentGameVersionRole, "parentGameVersion");
	roles.insert(RecommendedRole, "recommended");
	roles.insert(LatestRole, "latest");
	roles.insert(TypeRole, "type");
	roles.insert(BranchRole, "branch");
	roles.insert(PathRole, "path");
	roles.insert(ArchitectureRole, "architecture");
	return roles;
}
