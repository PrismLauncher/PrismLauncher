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

#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <pathutils.h>

#include "minecraft/MinecraftProfile.h"
#include "minecraft/VersionBuilder.h"
#include "ProfileUtils.h"
#include "NullProfileStrategy.h"

MinecraftProfile::MinecraftProfile(ProfileStrategy *strategy)
	: QAbstractListModel()
{
	setStrategy(strategy);
	clear();
}

void MinecraftProfile::setStrategy(ProfileStrategy* strategy)
{
	Q_ASSERT(strategy != nullptr);

	if(m_strategy != nullptr)
	{
		delete m_strategy;
		m_strategy = nullptr;
	}
	m_strategy = strategy;
	m_strategy->profile = this;
}

ProfileStrategy* MinecraftProfile::strategy()
{
	return m_strategy;
}

void MinecraftProfile::reload()
{
	beginResetModel();
	m_strategy->load();
	reapply();
	endResetModel();
}

void MinecraftProfile::clear()
{
	id.clear();
	m_updateTimeString.clear();
	m_updateTime = QDateTime();
	m_releaseTimeString.clear();
	m_releaseTime = QDateTime();
	type.clear();
	assets.clear();
	processArguments.clear();
	minecraftArguments.clear();
	minimumLauncherVersion = 0xDEADBEAF;
	mainClass.clear();
	appletClass.clear();
	libraries.clear();
	tweakers.clear();
	jarMods.clear();
	traits.clear();
}

void MinecraftProfile::clearPatches()
{
	beginResetModel();
	VersionPatches.clear();
	endResetModel();
}

void MinecraftProfile::appendPatch(ProfilePatchPtr patch)
{
	int index = VersionPatches.size();
	beginInsertRows(QModelIndex(), index, index);
	VersionPatches.append(patch);
	endInsertRows();
}

bool MinecraftProfile::canRemove(const int index) const
{
	return VersionPatches.at(index)->isMoveable();
}

bool MinecraftProfile::remove(const int index)
{
	if (!canRemove(index))
		return false;

	if(!m_strategy->removePatch(VersionPatches.at(index)))
	{
		return false;
	}

	beginRemoveRows(QModelIndex(), index, index);
	VersionPatches.removeAt(index);
	endRemoveRows();
	reapply();
	saveCurrentOrder();
	return true;
}

bool MinecraftProfile::remove(const QString id)
{
	int i = 0;
	for (auto patch : VersionPatches)
	{
		if (patch->getPatchID() == id)
		{
			return remove(i);
		}
		i++;
	}
	return false;
}

QString MinecraftProfile::versionFileId(const int index) const
{
	if (index < 0 || index >= VersionPatches.size())
	{
		return QString();
	}
	return VersionPatches.at(index)->getPatchID();
}

ProfilePatchPtr MinecraftProfile::versionPatch(const QString &id)
{
	for (auto file : VersionPatches)
	{
		if (file->getPatchID() == id)
		{
			return file;
		}
	}
	return 0;
}

ProfilePatchPtr MinecraftProfile::versionPatch(int index)
{
	if(index < 0 || index >= VersionPatches.size())
		return 0;
	return VersionPatches[index];
}

bool MinecraftProfile::isVanilla()
{
	for(auto patchptr: VersionPatches)
	{
		if(patchptr->isCustom())
			return false;
	}
	return true;
}

bool MinecraftProfile::revertToVanilla()
{
	/*
	beginResetModel();
	// remove patches, if present
	auto it = VersionPatches.begin();
	while (it != VersionPatches.end())
	{
		if ((*it)->isMoveable())
		{
			if(!preremove(*it))
			{
				endResetModel();
				saveCurrentOrder();
				return false;
			}
			if(!QFile::remove((*it)->getPatchFilename()))
			{
				endResetModel();
				saveCurrentOrder();
				return false;
			}
			it = VersionPatches.erase(it);
		}
		else
			it++;
	}
	reapply();
	endResetModel();
	saveCurrentOrder();
	return true;
	*/
	return false;
}

QList<std::shared_ptr<OneSixLibrary> > MinecraftProfile::getActiveNormalLibs()
{
	QList<std::shared_ptr<OneSixLibrary> > output;
	for (auto lib : libraries)
	{
		if (lib->isActive() && !lib->isNative())
		{
			for (auto other : output)
			{
				if (other->rawName() == lib->rawName())
				{
					qWarning() << "Multiple libraries with name" << lib->rawName() << "in library list!";
					continue;
				}
			}
			output.append(lib);
		}
	}
	return output;
}

QList<std::shared_ptr<OneSixLibrary> > MinecraftProfile::getActiveNativeLibs()
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

std::shared_ptr<MinecraftProfile> MinecraftProfile::fromJson(const QJsonObject &obj)
{
	std::shared_ptr<MinecraftProfile> version(new MinecraftProfile(new NullProfileStrategy()));
	try
	{
		VersionBuilder::readJsonAndApplyToVersion(version.get(), obj);
	}
	catch(MMCError & err)
	{
		return 0;
	}
	return version;
}

QVariant MinecraftProfile::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	int row = index.row();
	int column = index.column();

	if (row < 0 || row >= VersionPatches.size())
		return QVariant();

	if (role == Qt::DisplayRole)
	{
		switch (column)
		{
		case 0:
			return VersionPatches.at(row)->getPatchName();
		case 1:
			return VersionPatches.at(row)->getPatchVersion();
		default:
			return QVariant();
		}
	}
	return QVariant();
}
QVariant MinecraftProfile::headerData(int section, Qt::Orientation orientation, int role) const
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
Qt::ItemFlags MinecraftProfile::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;
	return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

int MinecraftProfile::rowCount(const QModelIndex &parent) const
{
	return VersionPatches.size();
}

int MinecraftProfile::columnCount(const QModelIndex &parent) const
{
	return 2;
}

void MinecraftProfile::saveCurrentOrder() const
{
	ProfileUtils::PatchOrder order;
	for(auto item: VersionPatches)
	{
		if(!item->isMoveable())
			continue;
		order.append(item->getPatchID());
	}
	m_strategy->saveOrder(order);
}

void MinecraftProfile::move(const int index, const MoveDirection direction)
{
	int theirIndex;
	if (direction == MoveUp)
	{
		theirIndex = index - 1;
	}
	else
	{
		theirIndex = index + 1;
	}

	if (index < 0 || index >= VersionPatches.size())
		return;
	if (theirIndex >= rowCount())
		theirIndex = rowCount() - 1;
	if (theirIndex == -1)
		theirIndex = rowCount() - 1;
	if (index == theirIndex)
		return;
	int togap = theirIndex > index ? theirIndex + 1 : theirIndex;

	auto from = versionPatch(index);
	auto to = versionPatch(theirIndex);

	if (!from || !to || !to->isMoveable() || !from->isMoveable())
	{
		return;
	}
	beginMoveRows(QModelIndex(), index, index, QModelIndex(), togap);
	VersionPatches.swap(index, theirIndex);
	endMoveRows();
	saveCurrentOrder();
	reapply();
}
void MinecraftProfile::resetOrder()
{
	m_strategy->resetOrder();
	reload();
}

void MinecraftProfile::reapply()
{
	clear();
	for(auto file: VersionPatches)
	{
		file->applyTo(this);
	}
	finalize();
}

void MinecraftProfile::finalize()
{
	// HACK: deny april fools. my head hurts enough already.
	QDate now = QDate::currentDate();
	bool isAprilFools = now.month() == 4 && now.day() == 1;
	if (assets.endsWith("_af") && !isAprilFools)
	{
		assets = assets.left(assets.length() - 3);
	}
	if (assets.isEmpty())
	{
		assets = "legacy";
	}
	auto finalizeArguments = [&]( QString & minecraftArguments, const QString & processArguments ) -> void
	{
		if (!minecraftArguments.isEmpty())
			return;
		QString toCompare = processArguments.toLower();
		if (toCompare == "legacy")
		{
			minecraftArguments = " ${auth_player_name} ${auth_session}";
		}
		else if (toCompare == "username_session")
		{
			minecraftArguments = "--username ${auth_player_name} --session ${auth_session}";
		}
		else if (toCompare == "username_session_version")
		{
			minecraftArguments = "--username ${auth_player_name} "
								"--session ${auth_session} "
								"--version ${profile_name}";
		}
	};
	finalizeArguments(vanillaMinecraftArguments, vanillaProcessArguments);
	finalizeArguments(minecraftArguments, processArguments);
}

void MinecraftProfile::installJarMods(QStringList selectedFiles)
{
	m_strategy->installJarMods(selectedFiles);
}

/*
 * TODO: get rid of this. Get rid of all order numbers.
 */
int MinecraftProfile::getFreeOrderNumber()
{
	int largest = 100;
	// yes, I do realize this is dumb. The order thing itself is dumb. and to be removed next.
	for(auto thing: VersionPatches)
	{
		int order = thing->getOrder();
		if(order > largest)
			largest = order;
	}
	return largest + 1;
}
