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
#include <QCryptographicHash>
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

MinecraftProfile::~MinecraftProfile()
{
	if(m_strategy)
	{
		delete m_strategy;
	}
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
	reapplyPatches();
	endResetModel();
}

void MinecraftProfile::clear()
{
	m_minecraftVersion.clear();
	m_minecraftVersionType.clear();
	m_minecraftAssets.reset();
	m_minecraftArguments.clear();
	m_tweakers.clear();
	m_mainClass.clear();
	m_appletClass.clear();
	m_libraries.clear();
	m_traits.clear();
	m_jarMods.clear();
	mojangDownloads.clear();
	m_problemSeverity = ProblemSeverity::PROBLEM_NONE;
}

void MinecraftProfile::clearPatches()
{
	beginResetModel();
	m_patches.clear();
	endResetModel();
}

void MinecraftProfile::appendPatch(ProfilePatchPtr patch)
{
	int index = m_patches.size();
	beginInsertRows(QModelIndex(), index, index);
	m_patches.append(patch);
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
	m_patches.removeAt(index);
	endRemoveRows();
	reapplyPatches();
	saveCurrentOrder();
	return true;
}

bool MinecraftProfile::remove(const QString id)
{
	int i = 0;
	for (auto patch : m_patches)
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
	reapplyPatches();
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
	reapplyPatches();
	saveCurrentOrder();
	// FIXME: maybe later in unstable
	// emit dataChanged(createIndex(index, 0), createIndex(index, columnCount(QModelIndex()) - 1));
	return true;
}

ProfilePatchPtr MinecraftProfile::versionPatch(const QString &id)
{
	for (auto file : m_patches)
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
	if(index < 0 || index >= m_patches.size())
		return nullptr;
	return m_patches[index];
}

bool MinecraftProfile::isVanilla()
{
	for(auto patchptr: m_patches)
	{
		if(patchptr->isCustom())
			return false;
	}
	return true;
}

bool MinecraftProfile::revertToVanilla()
{
	// remove patches, if present
	auto VersionPatchesCopy = m_patches;
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
				reapplyPatches();
				saveCurrentOrder();
				return false;
			}
		}
	}
	reapplyPatches();
	saveCurrentOrder();
	return true;
}

QVariant MinecraftProfile::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	int row = index.row();
	int column = index.column();

	if (row < 0 || row >= m_patches.size())
		return QVariant();

	auto patch = m_patches.at(row);

	if (role == Qt::DisplayRole)
	{
		switch (column)
		{
		case 0:
			return m_patches.at(row)->getName();
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
	return m_patches.size();
}

int MinecraftProfile::columnCount(const QModelIndex &parent) const
{
	return 2;
}

void MinecraftProfile::saveCurrentOrder() const
{
	ProfileUtils::PatchOrder order;
	for(auto item: m_patches)
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

	if (index < 0 || index >= m_patches.size())
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
	m_patches.swap(index, theirIndex);
	endMoveRows();
	reapplyPatches();
	saveCurrentOrder();
}
void MinecraftProfile::resetOrder()
{
	m_strategy->resetOrder();
	reload();
}

bool MinecraftProfile::reapplyPatches()
{
	try
	{
		clear();
		for(auto file: m_patches)
		{
			file->applyTo(this);
		}
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
	applyString(id, this->m_minecraftVersion);
}

void MinecraftProfile::applyAppletClass(const QString& appletClass)
{
	applyString(appletClass, this->m_appletClass);
}

void MinecraftProfile::applyMainClass(const QString& mainClass)
{
	applyString(mainClass, this->m_mainClass);
}

void MinecraftProfile::applyMinecraftArguments(const QString& minecraftArguments)
{
	applyString(minecraftArguments, this->m_minecraftArguments);
}

void MinecraftProfile::applyMinecraftVersionType(const QString& type)
{
	applyString(type, this->m_minecraftVersionType);
}

void MinecraftProfile::applyMinecraftAssets(MojangAssetIndexInfo::Ptr assets)
{
	if(assets)
	{
		m_minecraftAssets = assets;
	}
}

void MinecraftProfile::applyMojangDownload(const QString &key, MojangDownloadInfo::Ptr download)
{
	if(download)
	{
		mojangDownloads[key] = download;
	}
	else
	{
		mojangDownloads.remove(key);
	}
}

void MinecraftProfile::applyTraits(const QSet<QString>& traits)
{
	this->m_traits.unite(traits);
}

void MinecraftProfile::applyTweakers(const QStringList& tweakers)
{
	// FIXME: check for dupes?
	// FIXME: does order matter?
	for (auto tweaker : tweakers)
	{
		this->m_tweakers += tweaker;
	}
}

void MinecraftProfile::applyJarMods(const QList<JarmodPtr>& jarMods)
{
	this->m_jarMods.append(jarMods);
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
	if(!library->isActive())
	{
		return;
	}
	// find the library by name.
	const int index = findLibraryByName(m_libraries, library->rawName());
	// library not found? just add it.
	if (index < 0)
	{
		m_libraries.append(Library::limitedCopy(library));
		return;
	}
	auto existingLibrary = m_libraries.at(index);
	// if we are higher it means we should update
	if (Version(library->version()) > Version(existingLibrary->version()))
	{
		auto libraryCopy = Library::limitedCopy(library);
		m_libraries.replace(index, libraryCopy);
	}
}

void MinecraftProfile::applyProblemSeverity(ProblemSeverity severity)
{
	if (m_problemSeverity < severity)
	{
		m_problemSeverity = severity;
	}
}


QString MinecraftProfile::getMinecraftVersion() const
{
	return m_minecraftVersion;
}

QString MinecraftProfile::getAppletClass() const
{
	return m_appletClass;
}

QString MinecraftProfile::getMainClass() const
{
	return m_mainClass;
}

const QSet<QString> &MinecraftProfile::getTraits() const
{
	return m_traits;
}

const QStringList & MinecraftProfile::getTweakers() const
{
	return m_tweakers;
}

bool MinecraftProfile::hasTrait(const QString& trait) const
{
	return m_traits.contains(trait);
}

ProblemSeverity MinecraftProfile::getProblemSeverity() const
{
	return m_problemSeverity;
}

QString MinecraftProfile::getMinecraftVersionType() const
{
	return m_minecraftVersionType;
}

std::shared_ptr<MojangAssetIndexInfo> MinecraftProfile::getMinecraftAssets() const
{
	if(!m_minecraftAssets)
	{
		return std::make_shared<MojangAssetIndexInfo>("legacy");
	}
	return m_minecraftAssets;
}

QString MinecraftProfile::getMinecraftArguments() const
{
	return m_minecraftArguments;
}

const QList<JarmodPtr> & MinecraftProfile::getJarMods() const
{
	return m_jarMods;
}

const QList<LibraryPtr> & MinecraftProfile::getLibraries() const
{
	return m_libraries;
}

void MinecraftProfile::getLibraryFiles(const QString& architecture, QStringList& jars, QStringList& nativeJars) const
{
	QStringList native32, native64;
	jars.clear();
	nativeJars.clear();
	for (auto lib : getLibraries())
	{
		lib->getApplicableFiles(currentSystem, jars, nativeJars, native32, native64);
	}
	if(architecture == "32")
	{
		nativeJars.append(native32);
	}
	else if(architecture == "64")
	{
		nativeJars.append(native64);
	}
}


QString MinecraftProfile::getMainJarUrl() const
{
	auto iter = mojangDownloads.find("client");
	if(iter != mojangDownloads.end())
	{
		// current
		return iter.value()->url;
	}
	else
	{
		// legacy fallback
		return URLConstants::getLegacyJarUrl(getMinecraftVersion());
	}
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
	for(auto thing: m_patches)
	{
		int order = thing->getOrder();
		if(order > largest)
			largest = order;
	}
	return largest + 1;
}
