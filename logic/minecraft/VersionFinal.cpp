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

#include <QDebug>
#include <QFile>
#include <QDir>
#include <pathutils.h>

#include "logic/minecraft/VersionFinal.h"
#include "logic/minecraft/VersionBuilder.h"
#include "logic/OneSixInstance.h"

VersionFinal::VersionFinal(OneSixInstance *instance, QObject *parent)
	: QAbstractListModel(parent), m_instance(instance)
{
	clear();
}

void VersionFinal::reload(const QStringList &external)
{
	beginResetModel();
	VersionBuilder::build(this, m_instance, external);
	reapply(true);
	endResetModel();
}

void VersionFinal::clear()
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

bool VersionFinal::canRemove(const int index) const
{
	return VersionPatches.at(index)->isMoveable();
}

bool VersionFinal::preremove(VersionPatchPtr patch)
{
	bool ok = true;
	for(auto & jarmod: patch->getJarMods())
	{
		QString fullpath =PathCombine(m_instance->jarModsDir(), jarmod->name);
		QFileInfo finfo (fullpath);
		if(finfo.exists())
			ok &= QFile::remove(fullpath);
	}
	return ok;
}

bool VersionFinal::remove(const int index)
{
	if (!canRemove(index))
		return false;
	if(!preremove(VersionPatches[index]))
	{
		return false;
	}
	if(!QFile::remove(VersionPatches.at(index)->getPatchFilename()))
		return false;
	beginResetModel();
	VersionPatches.removeAt(index);
	reapply(true);
	endResetModel();
	return true;
}

bool VersionFinal::remove(const QString id)
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

QString VersionFinal::versionFileId(const int index) const
{
	if (index < 0 || index >= VersionPatches.size())
	{
		return QString();
	}
	return VersionPatches.at(index)->getPatchID();
}

VersionPatchPtr VersionFinal::versionPatch(const QString &id)
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

VersionPatchPtr VersionFinal::versionPatch(int index)
{
	if(index < 0 || index >= VersionPatches.size())
		return 0;
	return VersionPatches[index];
}


bool VersionFinal::hasJarMods()
{
	return !jarMods.isEmpty();
}

bool VersionFinal::hasFtbPack()
{
	return versionPatch("org.multimc.ftb.pack.json") != nullptr;
}

bool VersionFinal::removeFtbPack()
{
	return remove("org.multimc.ftb.pack.json");
}

bool VersionFinal::isVanilla()
{
	QDir patches(PathCombine(m_instance->instanceRoot(), "patches/"));
	if(VersionPatches.size() > 1)
		return false;
	if(QFile::exists(PathCombine(m_instance->instanceRoot(), "custom.json")))
		return false;
	return true;
}

bool VersionFinal::revertToVanilla()
{
	beginResetModel();
	auto it = VersionPatches.begin();
	while (it != VersionPatches.end())
	{
		if ((*it)->isMoveable())
		{
			if(!preremove(*it))
			{
				endResetModel();
				return false;
			}
			if(!QFile::remove((*it)->getPatchFilename()))
			{
				endResetModel();
				return false;
			}
			it = VersionPatches.erase(it);
		}
		else
			it++;
	}
	reapply(true);
	endResetModel();
	return true;
}

bool VersionFinal::usesLegacyCustomJson()
{
	return QFile::exists(PathCombine(m_instance->instanceRoot(), "custom.json"));
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
		VersionBuilder::readJsonAndApplyToVersion(version.get(), obj);
	}
	catch(MMCError & err)
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
	return VersionPatches.size();
}

int VersionFinal::columnCount(const QModelIndex &parent) const
{
	return 2;
}

QMap<QString, int> VersionFinal::getExistingOrder() const
{
	QMap<QString, int> order;
	// default
	{
		for (auto file : VersionPatches)
		{
			order.insert(file->getPatchID(), file->getOrder());
		}
	}
	// overriden
	{
		QMap<QString, int> overridenOrder = VersionBuilder::readOverrideOrders(m_instance);
		for (auto id : order.keys())
		{
			if (overridenOrder.contains(id))
			{
				order[id] = overridenOrder[id];
			}
		}
	}
	return order;
}

void VersionFinal::move(const int index, const MoveDirection direction)
{
	int theirIndex;
	int theirIndex_qt;
	if (direction == MoveUp)
	{
		theirIndex_qt = theirIndex = index - 1;
	}
	else
	{
		theirIndex = index + 1;
		theirIndex_qt = index + 2;
	}
	auto from = versionPatch(index);
	auto to = versionPatch(theirIndex);
	
	if (!from || !to || !from->isMoveable() || !from->isMoveable())
	{
		return;
	}
	if(direction == MoveDown)
	{
		beginMoveRows(QModelIndex(), index, index, QModelIndex(), theirIndex_qt);
	}
	else
	{
		beginMoveRows(QModelIndex(), index, index, QModelIndex(), theirIndex_qt);
	}
	VersionPatches.swap(index, theirIndex);
	endMoveRows();

	auto order = getExistingOrder();
	order[from->getPatchID()] = theirIndex;
	order[to->getPatchID()] = index;

	if (!VersionBuilder::writeOverrideOrders(order, m_instance))
	{
		throw MMCError(tr("Couldn't save the new order"));
	}
	else
	{
		reapply();
	}
}
void VersionFinal::resetOrder()
{
	QDir(m_instance->instanceRoot()).remove("order.json");
	reapply();
}

void VersionFinal::reapply(const bool alreadyReseting)
{
	if (!alreadyReseting)
	{
		beginResetModel();
	}

	clear();

	auto existingOrders = getExistingOrder();
	QList<int> orders = existingOrders.values();
	std::sort(orders.begin(), orders.end());
	QList<VersionPatchPtr> newVersionFiles;
	for (auto order : orders)
	{
		auto file = versionPatch(existingOrders.key(order));
		newVersionFiles.append(file);
		file->applyTo(this);
	}
	VersionPatches.swap(newVersionFiles);
	finalize();
	if (!alreadyReseting)
	{
		endResetModel();
	}
}

void VersionFinal::finalize()
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

