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

#include "VersionFinal.h"

#include <QDebug>
#include <QFile>

#include "OneSixVersionBuilder.h"

VersionFinal::VersionFinal(OneSixInstance *instance, QObject *parent)
	: QAbstractListModel(parent), m_instance(instance)
{
	clear();
}

bool VersionFinal::reload(const bool onlyVanilla, const QStringList &external)
{
	//FIXME: source of epic failure.
	beginResetModel();
	OneSixVersionBuilder::build(this, m_instance, onlyVanilla, external);
	endResetModel();
}

void VersionFinal::clear()
{
	beginResetModel();
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
	versionFiles.clear();
	endResetModel();
}

bool VersionFinal::canRemove(const int index) const
{
	if (index < versionFiles.size())
	{
		return versionFiles.at(index).id != "org.multimc.version.json";
	}
	return false;
}

QString VersionFinal::versionFileId(const int index) const
{
	if (index < 0 || index >= versionFiles.size())
	{
		return QString();
	}
	return versionFiles.at(index).id;
}

bool VersionFinal::remove(const int index)
{
	if (canRemove(index))
	{
		return QFile::remove(versionFiles.at(index).filename);
	}
	return false;
}

QList<std::shared_ptr<OneSixLibrary> > VersionFinal::getActiveNormalLibs()
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

QList<std::shared_ptr<OneSixLibrary> > VersionFinal::getActiveNativeLibs()
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

std::shared_ptr<VersionFinal> VersionFinal::fromJson(const QJsonObject &obj)
{
	std::shared_ptr<VersionFinal> version(new VersionFinal(0));
	try
	{
		OneSixVersionBuilder::readJsonAndApplyToVersion(version.get(), obj);
	}
	catch(MMCError err)
	{
		return 0;
	}
	return version;
}

QVariant VersionFinal::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	int row = index.row();
	int column = index.column();

	if (row < 0 || row >= versionFiles.size())
		return QVariant();

	if (role == Qt::DisplayRole)
	{
		switch (column)
		{
		case 0:
			return versionFiles.at(row).name;
		case 1:
			return versionFiles.at(row).version;
		default:
			return QVariant();
		}
	}
	return QVariant();
}

QVariant VersionFinal::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal)
	{
		if (role == Qt::DisplayRole)
		{
			switch (section)
			{
			case 0:
				return tr("Name");
			case 1:
				return tr("Version");
			default:
				return QVariant();
			}
		}
	}
	return QVariant();
}

Qt::ItemFlags VersionFinal::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;
	return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

int VersionFinal::rowCount(const QModelIndex &parent) const
{
	return versionFiles.size();
}

int VersionFinal::columnCount(const QModelIndex &parent) const
{
	return 2;
}
