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

#include "DerpVersion.h"

#include <QDebug>

#include "DerpVersionBuilder.h"

DerpVersion::DerpVersion(DerpInstance *instance, QObject *parent)
	: QAbstractListModel(parent), m_instance(instance)
{
}

bool DerpVersion::reload(QWidget *widgetParent)
{
	return DerpVersionBuilder::build(this, m_instance, widgetParent);
}

QList<std::shared_ptr<DerpLibrary> > DerpVersion::getActiveNormalLibs()
{
	QList<std::shared_ptr<DerpLibrary> > output;
	for (auto lib : libraries)
	{
		if (lib->isActive() && !lib->isNative())
		{
			output.append(lib);
		}
	}
	return output;
}

QList<std::shared_ptr<DerpLibrary> > DerpVersion::getActiveNativeLibs()
{
	QList<std::shared_ptr<DerpLibrary> > output;
	for (auto lib : libraries)
	{
		if (lib->isActive() && lib->isNative())
		{
			output.append(lib);
		}
	}
	return output;
}

QVariant DerpVersion::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	int row = index.row();
	int column = index.column();

	if (row < 0 || row >= libraries.size())
		return QVariant();

	if (role == Qt::DisplayRole)
	{
		switch (column)
		{
		case 0:
			return libraries[row]->name();
		case 1:
			return libraries[row]->type();
		case 2:
			return libraries[row]->version();
		default:
			return QVariant();
		}
	}
	return QVariant();
}

Qt::ItemFlags DerpVersion::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;
	int row = index.row();
	if (libraries[row]->isActive())
	{
		return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
	}
	else
	{
		return Qt::ItemNeverHasChildren;
	}
	// return QAbstractListModel::flags(index);
}

QVariant DerpVersion::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
		return QVariant();
	switch (section)
	{
	case 0:
		return QString("Name");
	case 1:
		return QString("Type");
	case 2:
		return QString("Version");
	default:
		return QString();
	}
}

int DerpVersion::rowCount(const QModelIndex &parent) const
{
	return libraries.size();
}

int DerpVersion::columnCount(const QModelIndex &parent) const
{
	return 3;
}

QDebug operator<<(QDebug &dbg, const DerpVersion *version)
{
	dbg.nospace() << "DerpVersion("
				  << "\n\tid=" << version->id
				  << "\n\ttime=" << version->time
				  << "\n\treleaseTime=" << version->releaseTime
				  << "\n\ttype=" << version->type
				  << "\n\tassets=" << version->assets
				  << "\n\tprocessArguments=" << version->processArguments
				  << "\n\tminecraftArguments=" << version->minecraftArguments
				  << "\n\tminimumLauncherVersion=" << version->minimumLauncherVersion
				  << "\n\tmainClass=" << version->mainClass
				  << "\n\tlibraries=";
	for (auto lib : version->libraries)
	{
		dbg.nospace() << "\n\t\t" << lib.get();
	}
	dbg.nospace() << "\n)";
	return dbg.maybeSpace();
}
QDebug operator<<(QDebug &dbg, const DerpLibrary *library)
{
	dbg.nospace() << "DerpLibrary("
				  << "\n\t\t\trawName=" << library->rawName()
				  << "\n\t\t\tname=" << library->name()
				  << "\n\t\t\tversion=" << library->version()
				  << "\n\t\t\ttype=" << library->type()
				  << "\n\t\t\tisActive=" << library->isActive()
				  << "\n\t\t\tisNative=" << library->isNative()
				  << "\n\t\t\tdownloadUrl=" << library->downloadUrl()
				  << "\n\t\t\tstoragePath=" << library->storagePath()
				  << "\n\t\t\tabsolutePath=" << library->absoluteUrl()
				  << "\n\t\t\thint=" << library->hint();
	dbg.nospace() << "\n\t\t)";
	return dbg.maybeSpace();
}
