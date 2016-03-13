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
#include <Version.h>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

#include "minecraft/MinecraftProfile.h"
#include "ProfileUtils.h"
#include "ProfileStrategy.h"
#include "Exception.h"

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
	reapplySafe();
	endResetModel();
}

void MinecraftProfile::clear()
{
	id.clear();
	type.clear();
	assets.clear();
	minecraftArguments.clear();
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

bool MinecraftProfile::remove(const int index)
{
	auto patch = versionPatch(index);
	if (!patch->isRemovable())
	{
		qDebug() << "Patch" << patch->getID() << "is non-removable";
		return false;
	}

	if(!m_strategy->removePatch(patch))
	{
		qCritical() << "Patch" << patch->getID() << "could not be removed";
		return false;
	}

	beginRemoveRows(QModelIndex(), index, index);
	VersionPatches.removeAt(index);
	endRemoveRows();
	reapplySafe();
	saveCurrentOrder();
	return true;
}

bool MinecraftProfile::remove(const QString id)
{
	int i = 0;
	for (auto patch : VersionPatches)
	{
		if (patch->getID() == id)
		{
			return remove(i);
		}
		i++;
	}
	return false;
}

bool MinecraftProfile::customize(int index)
{
	auto patch = versionPatch(index);
	if (!patch->isCustomizable())
	{
		qDebug() << "Patch" << patch->getID() << "is not customizable";
		return false;
	}
	if(!m_strategy->customizePatch(patch))
	{
		qCritical() << "Patch" << patch->getID() << "could not be customized";
		return false;
	}
	reapplySafe();
	saveCurrentOrder();
	// FIXME: maybe later in unstable
	// emit dataChanged(createIndex(index, 0), createIndex(index, columnCount(QModelIndex()) - 1));
	return true;
}

bool MinecraftProfile::revertToBase(int index)
{
	auto patch = versionPatch(index);
	if (!patch->isRevertible())
	{
		qDebug() << "Patch" << patch->getID() << "is not revertible";
		return false;
	}
	if(!m_strategy->revertPatch(patch))
	{
		qCritical() << "Patch" << patch->getID() << "could not be reverted";
		return false;
	}
	reapplySafe();
	saveCurrentOrder();
	// FIXME: maybe later in unstable
	// emit dataChanged(createIndex(index, 0), createIndex(index, columnCount(QModelIndex()) - 1));
	return true;
}

QString MinecraftProfile::versionFileId(const int index) const
{
	if (index < 0 || index >= VersionPatches.size())
	{
		return QString();
	}
	return VersionPatches.at(index)->getID();
}

ProfilePatchPtr MinecraftProfile::versionPatch(const QString &id)
{
	for (auto file : VersionPatches)
	{
		if (file->getID() == id)
		{
			return file;
		}
	}
	return nullptr;
}

ProfilePatchPtr MinecraftProfile::versionPatch(int index)
{
	if(index < 0 || index >= VersionPatches.size())
		return nullptr;
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
	// remove patches, if present
	auto VersionPatchesCopy = VersionPatches;
	for(auto & it: VersionPatchesCopy)
	{
		if (!it->isCustom())
		{
			continue;
		}
		if(it->isRevertible() || it->isRemovable())
		{
			if(!remove(it->getID()))
			{
				qWarning() << "Couldn't remove" << it->getID() << "from profile!";
				reapplySafe();
				saveCurrentOrder();
				return false;
			}
		}
	}
	reapplySafe();
	saveCurrentOrder();
	return true;
}

QVariant MinecraftProfile::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	int row = index.row();
	int column = index.column();

	if (row < 0 || row >= VersionPatches.size())
		return QVariant();

	auto patch = VersionPatches.at(row);

	if (role == Qt::DisplayRole)
	{
		switch (column)
		{
		case 0:
			return VersionPatches.at(row)->getName();
		case 1:
		{
			if(patch->isCustom())
			{
				return QString("%1 (Custom)").arg(patch->getVersion());
			}
			else
			{
				return patch->getVersion();
			}
		}
		default:
			return QVariant();
		}
	}
	if(role == Qt::DecorationRole)
	{
		switch(column)
		{
		case 0:
		{
			auto severity = patch->getProblemSeverity();
			switch (severity)
			{
				case PROBLEM_WARNING:
					return "warning";
				case PROBLEM_ERROR:
					return "error";
				default:
					return QVariant();
			}
		}
		default:
		{
			return QVariant();
		}
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
		order.append(item->getID());
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
	reapplySafe();
	saveCurrentOrder();
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
}

bool MinecraftProfile::reapplySafe()
{
	try
	{
		reapply();
	}
	catch (Exception & error)
	{
		clear();
		qWarning() << "Couldn't apply profile patches because: " << error.cause();
		return false;
	}
	return true;
}

static void applyString(const QString & from, QString & to)
{
	if(from.isEmpty())
		return;
	to = from;
}

void MinecraftProfile::applyMinecraftVersion(const QString& id)
{
	applyString(id, this->id);
}

void MinecraftProfile::applyAppletClass(const QString& appletClass)
{
	applyString(appletClass, this->appletClass);
}

void MinecraftProfile::applyMainClass(const QString& mainClass)
{
	applyString(mainClass, this->mainClass);
}

void MinecraftProfile::applyMinecraftArguments(const QString& minecraftArguments)
{
	applyString(minecraftArguments, this->minecraftArguments);
}

void MinecraftProfile::applyMinecraftVersionType(const QString& type)
{
	applyString(type, this->type);
}

void MinecraftProfile::applyMinecraftAssets(const QString& assets)
{
	applyString(assets, this->assets);
}

void MinecraftProfile::applyTraits(const QSet<QString>& traits)
{
	this->traits.unite(traits);
}

void MinecraftProfile::applyTweakers(const QStringList& tweakers)
{
	// FIXME: check for dupes?
	// FIXME: does order matter?
	for (auto tweaker : tweakers)
	{
		this->tweakers += tweaker;
	}
}

void MinecraftProfile::applyJarMods(const QList<JarmodPtr>& jarMods)
{
	this->jarMods.append(jarMods);
}

static int findLibraryByName(QList<LibraryPtr> haystack, const GradleSpecifier &needle)
{
	int retval = -1;
	for (int i = 0; i < haystack.size(); ++i)
	{
		if (haystack.at(i)->rawName().matchName(needle))
		{
			// only one is allowed.
			if (retval != -1)
				return -1;
			retval = i;
		}
	}
	return retval;
}

void MinecraftProfile::applyLibrary(LibraryPtr library)
{
	auto insert = [&](QList<LibraryPtr> & into)
	{
		// find the library by name.
		const int index = findLibraryByName(into, library->rawName());
		// library not found? just add it.
		if (index < 0)
		{
			into.append(Library::limitedCopy(library));
			return;
		}
		auto existingLibrary = into.at(index);
		// if we are higher it means we should update
		if (Version(library->version()) > Version(existingLibrary->version()))
		{
			auto libraryCopy = Library::limitedCopy(library);
			into.replace(index, libraryCopy);
		}
	};
	if(!library->isActive())
	{
		return;
	}
	if(library->isNative())
	{
		insert(nativeLibraries);
	}
	else
	{
		insert(libraries);
	}
}


QString MinecraftProfile::getMinecraftVersion() const
{
	return id;
}

QString MinecraftProfile::getAppletClass() const
{
	return appletClass;
}

QString MinecraftProfile::getMainClass() const
{
	return mainClass;
}

const QSet<QString> &MinecraftProfile::getTraits() const
{
	return traits;
}

const QStringList & MinecraftProfile::getTweakers() const
{
	return tweakers;
}

bool MinecraftProfile::hasTrait(const QString& trait) const
{
	return traits.contains(trait);
}


QString MinecraftProfile::getMinecraftVersionType() const
{
	return type;
}

QString MinecraftProfile::getMinecraftAssets() const
{
	// HACK: deny april fools. my head hurts enough already.
	QDate now = QDate::currentDate();
	bool isAprilFools = now.month() == 4 && now.day() == 1;
	if (assets.endsWith("_af") && !isAprilFools)
	{
		return assets.left(assets.length() - 3);
	}
	if (assets.isEmpty())
	{
		return QLatin1Literal("legacy");
	}
	return assets;
}

QString MinecraftProfile::getMinecraftArguments() const
{
	return minecraftArguments;
}

const QList<JarmodPtr> & MinecraftProfile::getJarMods() const
{
	return jarMods;
}

const QList<LibraryPtr> & MinecraftProfile::getLibraries() const
{
	return libraries;
}

const QList<LibraryPtr> & MinecraftProfile::getNativeLibraries() const
{
	return nativeLibraries;
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
