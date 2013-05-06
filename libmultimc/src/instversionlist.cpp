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

#include "instversionlist.h"
#include "instversion.h"

InstVersionList::InstVersionList(QObject *parent) :
	QAbstractListModel(parent)
{
}

const InstVersion *InstVersionList::findVersion(const QString &descriptor)
{
	for (int i = 0; i < count(); i++)
	{
		if (at(i)->descriptor() == descriptor)
			return at(i);
	}
	return NULL;
}

const InstVersion *InstVersionList::getLatestStable()
{
	if (count() <= 0)
		return NULL;
	else
		return at(0);
}

QVariant InstVersionList::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();
	
	if (index.row() > count())
		return QVariant();
	
	
	const InstVersion *version = at(index.row());
	
	switch (role)
	{
	case Qt::DisplayRole:
		switch (index.column())
		{
		case NameColumn:
			return version->name();
			
		case TypeColumn:
			return version->typeName();
			
		case TimeColumn:
			return version->timestamp();
			
		default:
			return QVariant();
		}
		
	case Qt::ToolTipRole:
		return version->descriptor();
		
	case VersionPointerRole:
		return qVariantFromValue((void *) version);
		
	default:
		return QVariant();
	}
}

QVariant InstVersionList::headerData(int section, Qt::Orientation orientation, int role) const
{
	switch (role)
	{
	case Qt::DisplayRole:
		switch (section)
		{
		case NameColumn:
			return "Name";
			
		case TypeColumn:
			return "Type";
			
		case TimeColumn:
			return "Time";
		
		default:
			return QVariant();
		}
		
	case Qt::ToolTipRole:
		switch (section)
		{
		case NameColumn:
			return "The name of the version.";
			
		case TypeColumn:
			return "The version's type.";
		
		default:
			return QVariant();
		}
		
	default:
		return QVariant();
	}
}

int InstVersionList::rowCount(const QModelIndex &parent) const
{
	// Return count
	return count();
}

int InstVersionList::columnCount(const QModelIndex &parent) const
{
	return 2;
}
