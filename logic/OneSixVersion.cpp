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

#include "OneSixVersion.h"

#include <QDebug>

#include "OneSixVersionBuilder.h"

OneSixVersion::OneSixVersion(OneSixInstance *instance, QObject *parent)
	: QAbstractListModel(parent), m_instance(instance)
{
	clear();
}

bool OneSixVersion::reload(QWidget *widgetParent, const bool excludeCustom)
{
	return OneSixVersionBuilder::build(this, m_instance, widgetParent, excludeCustom);
}

void OneSixVersion::clear()
{
	id.clear();
	time.clear();
	releaseTime.clear();
	type.clear();
	assets.clear();
	processArguments.clear();
	minecraftArguments.clear();
	minimumLauncherVersion = 0xDEADBEAF;
	mainClass.clear();
	libraries.clear();
	tweakers.clear();
}

void OneSixVersion::dump() const
{
	qDebug().nospace() << "OneSixVersion("
				  << "\n\tid=" << id
				  << "\n\ttime=" << time
				  << "\n\treleaseTime=" << releaseTime
				  << "\n\ttype=" << type
				  << "\n\tassets=" << assets
				  << "\n\tprocessArguments=" << processArguments
				  << "\n\tminecraftArguments=" << minecraftArguments
				  << "\n\tminimumLauncherVersion=" << minimumLauncherVersion
				  << "\n\tmainClass=" << mainClass
				  << "\n\tlibraries=";
	for (auto lib : libraries)
	{
		qDebug().nospace() << "\n\t\t" << lib.get();
	}
	qDebug().nospace() << "\n)";
}

QList<std::shared_ptr<OneSixLibrary> > OneSixVersion::getActiveNormalLibs()
{
	QList<std::shared_ptr<OneSixLibrary> > output;
	for (auto lib : libraries)
	{
		if (lib->isActive() && !lib->isNative())
		{
			output.append(lib);
		}
	}
	return output;
}

QList<std::shared_ptr<OneSixLibrary> > OneSixVersion::getActiveNativeLibs()
{
	QList<std::shared_ptr<OneSixLibrary> > output;
	for (auto lib : libraries)
	{
		if (lib->isActive() && lib->isNative())
		{
			output.append(lib);
		}
	}
	return output;
}

std::shared_ptr<OneSixVersion> OneSixVersion::fromJson(const QJsonObject &obj)
{
	std::shared_ptr<OneSixVersion> version(new OneSixVersion(0));
	if (OneSixVersionBuilder::read(version.get(), obj))
	{
		return version;
	}
	return 0;
}

QVariant OneSixVersion::data(const QModelIndex &index, int role) const
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

Qt::ItemFlags OneSixVersion::flags(const QModelIndex &index) const
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

QVariant OneSixVersion::headerData(int section, Qt::Orientation orientation, int role) const
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

int OneSixVersion::rowCount(const QModelIndex &parent) const
{
	return libraries.size();
}

int OneSixVersion::columnCount(const QModelIndex &parent) const
{
	return 3;
}

QDebug operator<<(QDebug &dbg, const OneSixVersion *version)
{
	version->dump();
	return dbg.maybeSpace();
}
QDebug operator<<(QDebug &dbg, const OneSixLibrary *library)
{
	dbg.nospace() << "OneSixLibrary("
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
