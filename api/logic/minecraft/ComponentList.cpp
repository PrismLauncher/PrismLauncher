/* Copyright 2013-2017 MultiMC Contributors
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

#include "Exception.h"
#include <minecraft/OneSixVersionFormat.h>
#include <FileSystem.h>
#include <QSaveFile>
#include <Env.h>
#include <meta/Index.h>
#include <minecraft/MinecraftInstance.h>
#include <QUuid>
#include <QTimer>
#include <Json.h>

#include "ComponentList.h"
#include "ComponentList_p.h"
#include "ComponentUpdateTask.h"

ComponentList::ComponentList(MinecraftInstance * instance)
	: QAbstractListModel()
{
	d.reset(new ComponentListData);
	d->m_instance = instance;
	d->m_saveTimer.setSingleShot(true);
	d->m_saveTimer.setInterval(5000);
	connect(&d->m_saveTimer, &QTimer::timeout, this, &ComponentList::save_internal);
}

ComponentList::~ComponentList()
{
	saveNow();
}

// BEGIN: component file format

static const int currentComponentsFileVersion = 1;

static QJsonObject componentToJsonV1(ComponentPtr component)
{
	QJsonObject obj;
	// critical
	obj.insert("uid", component->m_uid);
	if(!component->m_version.isEmpty())
	{
		obj.insert("version", component->m_version);
	}
	if(component->m_dependencyOnly)
	{
		obj.insert("dependencyOnly", true);
	}
	if(component->m_important)
	{
		obj.insert("important", true);
	}
	if(component->m_disabled)
	{
		obj.insert("disabled", true);
	}

	// cached
	if(!component->m_cachedVersion.isEmpty())
	{
		obj.insert("cachedVersion", component->m_cachedVersion);
	}
	if(!component->m_cachedName.isEmpty())
	{
		obj.insert("cachedName", component->m_cachedName);
	}
	Meta::serializeRequires(obj, &component->m_cachedRequires, "cachedRequires");
	Meta::serializeRequires(obj, &component->m_cachedConflicts, "cachedConflicts");
	if(component->m_cachedVolatile)
	{
		obj.insert("cachedVolatile", true);
	}
	return obj;
}

static ComponentPtr componentFromJsonV1(ComponentList * parent, const QString & componentJsonPattern, const QJsonObject &obj)
{
	// critical
	auto uid = Json::requireString(obj.value("uid"));
	auto filePath = componentJsonPattern.arg(uid);
	auto component = new Component(parent, uid);
	component->m_version = Json::ensureString(obj.value("version"));
	component->m_dependencyOnly = Json::ensureBoolean(obj.value("dependencyOnly"), false);
	component->m_important = Json::ensureBoolean(obj.value("important"), false);

	// cached
	// TODO @RESILIENCE: ignore invalid values/structure here?
	component->m_cachedVersion = Json::ensureString(obj.value("cachedVersion"));
	component->m_cachedName = Json::ensureString(obj.value("cachedName"));
	Meta::parseRequires(obj, &component->m_cachedRequires, "cachedRequires");
	Meta::parseRequires(obj, &component->m_cachedConflicts, "cachedConflicts");
	component->m_cachedVolatile = Json::ensureBoolean(obj.value("volatile"), false);
	bool disabled = Json::ensureBoolean(obj.value("disabled"), false);
	component->setEnabled(!disabled);
	return component;
}

// Save the given component container data to a file
static bool saveComponentList(const QString & filename, const ComponentContainer & container)
{
	QJsonObject obj;
	obj.insert("formatVersion", currentComponentsFileVersion);
	QJsonArray orderArray;
	for(auto component: container)
	{
		orderArray.append(componentToJsonV1(component));
	}
	obj.insert("components", orderArray);
	QSaveFile outFile(filename);
	if (!outFile.open(QFile::WriteOnly))
	{
		qCritical() << "Couldn't open" << outFile.fileName()
					 << "for writing:" << outFile.errorString();
		return false;
	}
	auto data = QJsonDocument(obj).toJson(QJsonDocument::Indented);
	if(outFile.write(data) != data.size())
	{
		qCritical() << "Couldn't write all the data into" << outFile.fileName()
					 << "because:" << outFile.errorString();
		return false;
	}
	if(!outFile.commit())
	{
		qCritical() << "Couldn't save" << outFile.fileName()
					 << "because:" << outFile.errorString();
	}
	return true;
}

// Read the given file into component containers
static bool loadComponentList(ComponentList * parent, const QString & filename, const QString & componentJsonPattern, ComponentContainer & container)
{
	QFile componentsFile(filename);
	if (!componentsFile.exists())
	{
		qWarning() << "Components file doesn't exist. This should never happen.";
		return false;
	}
	if (!componentsFile.open(QFile::ReadOnly))
	{
		qCritical() << "Couldn't open" << componentsFile.fileName()
					 << " for reading:" << componentsFile.errorString();
		qWarning() << "Ignoring overriden order";
		return false;
	}

	// and it's valid JSON
	QJsonParseError error;
	QJsonDocument doc = QJsonDocument::fromJson(componentsFile.readAll(), &error);
	if (error.error != QJsonParseError::NoError)
	{
		qCritical() << "Couldn't parse" << componentsFile.fileName() << ":" << error.errorString();
		qWarning() << "Ignoring overriden order";
		return false;
	}

	// and then read it and process it if all above is true.
	try
	{
		auto obj = Json::requireObject(doc);
		// check order file version.
		auto version = Json::requireInteger(obj.value("formatVersion"));
		if (version != currentComponentsFileVersion)
		{
			throw JSONValidationError(QObject::tr("Invalid component file version, expected %1")
										  .arg(currentComponentsFileVersion));
		}
		auto orderArray = Json::requireArray(obj.value("components"));
		for(auto item: orderArray)
		{
			auto obj = Json::requireObject(item, "Component must be an object.");
			container.append(componentFromJsonV1(parent, componentJsonPattern, obj));
		}
	}
	catch (JSONValidationError &err)
	{
		qCritical() << "Couldn't parse" << componentsFile.fileName() << ": bad file format";
		container.clear();
		return false;
	}
	return true;
}

// END: component file format

// BEGIN: save/load logic

void ComponentList::saveNow()
{
	if(saveIsScheduled())
	{
		d->m_saveTimer.stop();
		save_internal();
	}
}

bool ComponentList::saveIsScheduled() const
{
	return d->dirty;
}

void ComponentList::buildingFromScratch()
{
	d->loaded = true;
	d->dirty = true;
}

void ComponentList::scheduleSave()
{
	if(!d->loaded)
	{
		qDebug() << "Component list should never save if it didn't successfully load, instance:" << d->m_instance->name();
		return;
	}
	if(!d->dirty)
	{
		d->dirty = true;
		qDebug() << "Component list save is scheduled for" << d->m_instance->name();
	}
	d->m_saveTimer.start();
}

QString ComponentList::componentsFilePath() const
{
	return FS::PathCombine(d->m_instance->instanceRoot(), "mmc-pack.json");
}

QString ComponentList::patchesPattern() const
{
	return FS::PathCombine(d->m_instance->instanceRoot(), "patches", "%1.json");
}

QString ComponentList::patchFilePathForUid(const QString& uid) const
{
	return patchesPattern().arg(uid);
}

void ComponentList::save_internal()
{
	qDebug() << "Component list save performed now for" << d->m_instance->name();
	auto filename = componentsFilePath();
	saveComponentList(filename, d->components);
	d->dirty = false;
}

bool ComponentList::load()
{
	auto filename = componentsFilePath();
	QFile componentsFile(filename);

	// migrate old config to new one, if needed
	if(!componentsFile.exists())
	{
		if(!migratePreComponentConfig())
		{
			// FIXME: the user should be notified...
			qCritical() << "Failed to convert old pre-component config for instance" << d->m_instance->name();
			return false;
		}
	}

	// load the new component list and swap it with the current one...
	ComponentContainer newComponents;
	if(!loadComponentList(this, filename, patchesPattern(), newComponents))
	{
		qCritical() << "Failed to load the component config for instance" << d->m_instance->name();
		return false;
	}
	else
	{
		// FIXME: actually use fine-grained updates, not this...
		beginResetModel();
		// disconnect all the old components
		for(auto component: d->components)
		{
			disconnect(component.get(), &Component::dataChanged, this, &ComponentList::componentDataChanged);
		}
		d->components.clear();
		d->componentIndex.clear();
		for(auto component: newComponents)
		{
			if(d->componentIndex.contains(component->m_uid))
			{
				qWarning() << "Ignoring duplicate component entry" << component->m_uid;
				continue;
			}
			connect(component.get(), &Component::dataChanged, this, &ComponentList::componentDataChanged);
			d->components.append(component);
			d->componentIndex[component->m_uid] = component;
		}
		endResetModel();
		d->loaded = true;
		return true;
	}
}

void ComponentList::reload(Net::Mode netmode)
{
	// Do not reload when the update/resolve task is running. It is in control.
	if(d->m_updateTask)
	{
		return;
	}

	// flush any scheduled saves to not lose state
	saveNow();

	// FIXME: differentiate when a reapply is required by propagating state from components
	invalidateLaunchProfile();

	if(load())
	{
		resolve(netmode);
	}
}

shared_qobject_ptr<Task> ComponentList::getCurrentTask()
{
	return d->m_updateTask;
}

void ComponentList::resolve(Net::Mode netmode)
{
	auto updateTask = new ComponentUpdateTask(ComponentUpdateTask::Mode::Resolution, netmode, this);
	d->m_updateTask.reset(updateTask);
	connect(updateTask, &ComponentUpdateTask::succeeded, this, &ComponentList::updateSucceeded);
	connect(updateTask, &ComponentUpdateTask::failed, this, &ComponentList::updateFailed);
	d->m_updateTask->start();
}


void ComponentList::updateSucceeded()
{
	qDebug() << "Component list update/resolve task succeeded for" << d->m_instance->name();
	d->m_updateTask.reset();
	invalidateLaunchProfile();
}

void ComponentList::updateFailed(const QString& error)
{
	qDebug() << "Component list update/resolve task failed for" << d->m_instance->name() << "Reason:" << error;
	d->m_updateTask.reset();
	invalidateLaunchProfile();
}

// NOTE this is really old stuff, and only needs to be used when loading the old hardcoded component-unaware format (loadPreComponentConfig).
static void upgradeDeprecatedFiles(QString root, QString instanceName)
{
	auto versionJsonPath = FS::PathCombine(root, "version.json");
	auto customJsonPath = FS::PathCombine(root, "custom.json");
	auto mcJson = FS::PathCombine(root, "patches" , "net.minecraft.json");

	QString sourceFile;
	QString renameFile;

	// convert old crap.
	if(QFile::exists(customJsonPath))
	{
		sourceFile = customJsonPath;
		renameFile = versionJsonPath;
	}
	else if(QFile::exists(versionJsonPath))
	{
		sourceFile = versionJsonPath;
	}
	if(!sourceFile.isEmpty() && !QFile::exists(mcJson))
	{
		if(!FS::ensureFilePathExists(mcJson))
		{
			qWarning() << "Couldn't create patches folder for" << instanceName;
			return;
		}
		if(!renameFile.isEmpty() && QFile::exists(renameFile))
		{
			if(!QFile::rename(renameFile, renameFile + ".old"))
			{
				qWarning() << "Couldn't rename" << renameFile << "to" << renameFile + ".old" << "in" << instanceName;
				return;
			}
		}
		auto file = ProfileUtils::parseJsonFile(QFileInfo(sourceFile), false);
		ProfileUtils::removeLwjglFromPatch(file);
		file->uid = "net.minecraft";
		file->version = file->minecraftVersion;
		file->name = "Minecraft";

		Meta::Require needsLwjgl;
		needsLwjgl.uid = "org.lwjgl";
		file->requires.insert(needsLwjgl);

		if(!ProfileUtils::saveJsonFile(OneSixVersionFormat::versionFileToJson(file), mcJson))
		{
			return;
		}
		if(!QFile::rename(sourceFile, sourceFile + ".old"))
		{
			qWarning() << "Couldn't rename" << sourceFile << "to" << sourceFile + ".old" << "in" << instanceName;
			return;
		}
	}
}

/*
 * Migrate old layout to the component based one...
 * - Part of the version information is taken from `instance.cfg` (fed to this class from outside).
 * - Part is taken from the old order.json file.
 * - Part is loaded from loose json files in the instance's `patches` directory.
 */
bool ComponentList::migratePreComponentConfig()
{
	// upgrade the very old files from the beginnings of MultiMC 5
	upgradeDeprecatedFiles(d->m_instance->instanceRoot(), d->m_instance->name());

	QList<ComponentPtr> components;
	QSet<QString> loaded;

	auto addBuiltinPatch = [&](const QString &uid, bool asDependency, const QString & emptyVersion, const Meta::Require & req, const Meta::Require & conflict)
	{
		auto jsonFilePath = FS::PathCombine(d->m_instance->instanceRoot(), "patches" , uid + ".json");
		auto intendedVersion = d->getOldConfigVersion(uid);
		// load up the base minecraft patch
		ComponentPtr component;
		if(QFile::exists(jsonFilePath))
		{
			if(intendedVersion.isEmpty())
			{
				intendedVersion = emptyVersion;
			}
			auto file = ProfileUtils::parseJsonFile(QFileInfo(jsonFilePath), false);
			// fix uid
			file->uid = uid;
			// if version is missing, add it from the outside.
			if(file->version.isEmpty())
			{
				file->version = intendedVersion;
			}
			// if this is a dependency (LWJGL), mark it also as volatile
			if(asDependency)
			{
				file->m_volatile = true;
			}
			// insert requirements if needed
			if(!req.uid.isEmpty())
			{
				file->requires.insert(req);
			}
			// insert conflicts if needed
			if(!conflict.uid.isEmpty())
			{
				file->conflicts.insert(conflict);
			}
			// FIXME: @QUALITY do not ignore return value
			ProfileUtils::saveJsonFile(OneSixVersionFormat::versionFileToJson(file), jsonFilePath);
			component = new Component(this, uid, file);
			component->m_version = intendedVersion;
		}
		else if(!intendedVersion.isEmpty())
		{
			auto metaVersion = ENV.metadataIndex()->get(uid, intendedVersion);
			component = new Component(this, metaVersion);
		}
		else
		{
			return;
		}
		component->m_dependencyOnly = asDependency;
		component->m_important = !asDependency;
		components.append(component);
	};
	// TODO: insert depends and conflicts here if these are customized files...
	Meta::Require reqLwjgl;
	reqLwjgl.uid = "org.lwjgl";
	reqLwjgl.suggests = "2.9.1";
	Meta::Require conflictLwjgl3;
	conflictLwjgl3.uid = "org.lwjgl3";
	Meta::Require nullReq;
	addBuiltinPatch("org.lwjgl", true, "2.9.1", nullReq, conflictLwjgl3);
	addBuiltinPatch("net.minecraft", false, QString(), reqLwjgl, nullReq);

	// first, collect all other file-based patches and load them
	QMap<QString, ComponentPtr> loadedComponents;
	QDir patchesDir(FS::PathCombine(d->m_instance->instanceRoot(),"patches"));
	for (auto info : patchesDir.entryInfoList(QStringList() << "*.json", QDir::Files))
	{
		// parse the file
		qDebug() << "Reading" << info.fileName();
		auto file = ProfileUtils::parseJsonFile(info, true);

		// correct missing or wrong uid based on the file name
		QString uid = info.completeBaseName();

		// ignore builtins, they've been handled already
		if (uid == "net.minecraft")
			continue;
		if (uid == "org.lwjgl")
			continue;

		// handle horrible corner cases
		if(uid.isEmpty())
		{
			// if you have a file named '.json', make it just go away.
			// FIXME: @QUALITY do not ignore return value
			QFile::remove(info.absoluteFilePath());
			continue;
		}
		file->uid = uid;
		// FIXME: @QUALITY do not ignore return value
		ProfileUtils::saveJsonFile(OneSixVersionFormat::versionFileToJson(file), info.absoluteFilePath());

		auto component = new Component(this, file->uid, file);
		auto version = d->getOldConfigVersion(file->uid);
		if(!version.isEmpty())
		{
			component->m_version = version;
		}
		loadedComponents[file->uid] = component;
	}
	// try to load the other 'hardcoded' patches (forge, liteloader), if they weren't loaded from files
	auto loadSpecial = [&](const QString & uid, int order)
	{
		auto patchVersion = d->getOldConfigVersion(uid);
		if(!patchVersion.isEmpty() && !loadedComponents.contains(uid))
		{
			auto patch = new Component(this, ENV.metadataIndex()->get(uid, patchVersion));
			patch->setOrder(order);
			loadedComponents[uid] = patch;
		}
	};
	loadSpecial("net.minecraftforge", 5);
	loadSpecial("com.mumfrey.liteloader", 10);

	// load the old order.json file, if present
	ProfileUtils::PatchOrder userOrder;
	ProfileUtils::readOverrideOrders(FS::PathCombine(d->m_instance->instanceRoot(), "order.json"), userOrder);

	// now add all the patches by user sort order
	for (auto uid : userOrder)
	{
		// ignore builtins
		if (uid == "net.minecraft")
			continue;
		if (uid == "org.lwjgl")
			continue;
		// ordering has a patch that is gone?
		if(!loadedComponents.contains(uid))
		{
			continue;
		}
		components.append(loadedComponents.take(uid));
	}

	// is there anything left to sort? - this is used when there are leftover components that aren't part of the order.json
	if(!loadedComponents.isEmpty())
	{
		// inserting into multimap by order number as key sorts the patches and detects duplicates
		QMultiMap<int, ComponentPtr> files;
		auto iter = loadedComponents.begin();
		while(iter != loadedComponents.end())
		{
			files.insert((*iter)->getOrder(), *iter);
			iter++;
		}

		// then just extract the patches and put them in the list
		for (auto order : files.keys())
		{
			const auto &values = files.values(order);
			for(auto &value: values)
			{
				// TODO: put back the insertion of problem messages here, so the user knows about the id duplication
				components.append(value);
			}
		}
	}
	// new we have a complete list of components...
	return saveComponentList(componentsFilePath(), components);
}

// END: save/load

void ComponentList::appendComponent(ComponentPtr component)
{
	insertComponent(d->components.size(), component);
}

void ComponentList::insertComponent(size_t index, ComponentPtr component)
{
	auto id = component->getID();
	if(id.isEmpty())
	{
		qWarning() << "Attempt to add a component with empty ID!";
		return;
	}
	if(d->componentIndex.contains(id))
	{
		qWarning() << "Attempt to add a component that is already present!";
		return;
	}
	beginInsertRows(QModelIndex(), index, index);
	d->components.insert(index, component);
	d->componentIndex[id] = component;
	endInsertRows();
	connect(component.get(), &Component::dataChanged, this, &ComponentList::componentDataChanged);
	scheduleSave();
}

void ComponentList::componentDataChanged()
{
	auto objPtr = qobject_cast<Component *>(sender());
	if(!objPtr)
	{
		qWarning() << "ComponentList got dataChenged signal from a non-Component!";
		return;
	}
	// figure out which one is it... in a seriously dumb way.
	int index = 0;
	for (auto component: d->components)
	{
		if(component.get() == objPtr)
		{
			emit dataChanged(createIndex(index, 0), createIndex(index, columnCount(QModelIndex()) - 1));
			scheduleSave();
			return;
		}
		index++;
	}
	qWarning() << "ComponentList got dataChenged signal from a Component which does not belong to it!";
}

bool ComponentList::remove(const int index)
{
	auto patch = getComponent(index);
	if (!patch->isRemovable())
	{
		qWarning() << "Patch" << patch->getID() << "is non-removable";
		return false;
	}

	if(!removeComponent_internal(patch))
	{
		qCritical() << "Patch" << patch->getID() << "could not be removed";
		return false;
	}

	beginRemoveRows(QModelIndex(), index, index);
	d->components.removeAt(index);
	d->componentIndex.remove(patch->getID());
	endRemoveRows();
	invalidateLaunchProfile();
	scheduleSave();
	return true;
}

bool ComponentList::remove(const QString id)
{
	int i = 0;
	for (auto patch : d->components)
	{
		if (patch->getID() == id)
		{
			return remove(i);
		}
		i++;
	}
	return false;
}

bool ComponentList::customize(int index)
{
	auto patch = getComponent(index);
	if (!patch->isCustomizable())
	{
		qDebug() << "Patch" << patch->getID() << "is not customizable";
		return false;
	}
	if(!patch->customize())
	{
		qCritical() << "Patch" << patch->getID() << "could not be customized";
		return false;
	}
	invalidateLaunchProfile();
	scheduleSave();
	return true;
}

bool ComponentList::revertToBase(int index)
{
	auto patch = getComponent(index);
	if (!patch->isRevertible())
	{
		qDebug() << "Patch" << patch->getID() << "is not revertible";
		return false;
	}
	if(!patch->revert())
	{
		qCritical() << "Patch" << patch->getID() << "could not be reverted";
		return false;
	}
	invalidateLaunchProfile();
	scheduleSave();
	return true;
}

Component * ComponentList::getComponent(const QString &id)
{
	auto iter = d->componentIndex.find(id);
	if (iter == d->componentIndex.end())
	{
		return nullptr;
	}
	return (*iter).get();
}

Component * ComponentList::getComponent(int index)
{
	if(index < 0 || index >= d->components.size())
	{
		return nullptr;
	}
	return d->components[index].get();
}

bool ComponentList::isVanilla()
{
	for(auto patchptr: d->components)
	{
		if(patchptr->isCustom())
			return false;
	}
	return true;
}

bool ComponentList::revertToVanilla()
{
	// remove patches, if present
	auto VersionPatchesCopy = d->components;
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
				invalidateLaunchProfile();
				scheduleSave();
				return false;
			}
		}
	}
	invalidateLaunchProfile();
	scheduleSave();
	return true;
}

QVariant ComponentList::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	int row = index.row();
	int column = index.column();

	if (row < 0 || row >= d->components.size())
		return QVariant();

	auto patch = d->components.at(row);

	switch (role)
	{
	case Qt::CheckStateRole:
	{
		switch (column)
		{
			case NameColumn:
				return d->components.at(row)->isEnabled() ? Qt::Checked : Qt::Unchecked;
			default:
				return QVariant();
		}
	}
	case Qt::DisplayRole:
	{
		switch (column)
		{
		case NameColumn:
			return d->components.at(row)->getName();
		case VersionColumn:
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
	case Qt::DecorationRole:
	{
		switch(column)
		{
		case NameColumn:
		{
			auto severity = patch->getProblemSeverity();
			switch (severity)
			{
				case ProblemSeverity::Warning:
					return "warning";
				case ProblemSeverity::Error:
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
	}
	return QVariant();
}

bool ComponentList::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if (!index.isValid() || index.row() < 0 || index.row() >= rowCount(index))
	{
		return false;
	}

	if (role == Qt::CheckStateRole)
	{
		auto component = d->components[index.row()];
		if (component->setEnabled(!component->isEnabled()))
		{
			return true;
		}
	}
	return false;
}

QVariant ComponentList::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal)
	{
		if (role == Qt::DisplayRole)
		{
			switch (section)
			{
			case NameColumn:
				return tr("Name");
			case VersionColumn:
				return tr("Version");
			default:
				return QVariant();
			}
		}
	}
	return QVariant();
}
Qt::ItemFlags ComponentList::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;

	Qt::ItemFlags outFlags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

	int row = index.row();

	if (row < 0 || row >= d->components.size())
		return Qt::NoItemFlags;

	auto patch = d->components.at(row);
	// TODO: this will need fine-tuning later...
	if(patch->canBeDisabled())
	{
		outFlags |= Qt::ItemIsUserCheckable;
	}
	return outFlags;
}

int ComponentList::rowCount(const QModelIndex &parent) const
{
	return d->components.size();
}

int ComponentList::columnCount(const QModelIndex &parent) const
{
	return NUM_COLUMNS;
}

void ComponentList::move(const int index, const MoveDirection direction)
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

	if (index < 0 || index >= d->components.size())
		return;
	if (theirIndex >= rowCount())
		theirIndex = rowCount() - 1;
	if (theirIndex == -1)
		theirIndex = rowCount() - 1;
	if (index == theirIndex)
		return;
	int togap = theirIndex > index ? theirIndex + 1 : theirIndex;

	auto from = getComponent(index);
	auto to = getComponent(theirIndex);

	if (!from || !to || !to->isMoveable() || !from->isMoveable())
	{
		return;
	}
	beginMoveRows(QModelIndex(), index, index, QModelIndex(), togap);
	d->components.swap(index, theirIndex);
	endMoveRows();
	invalidateLaunchProfile();
	scheduleSave();
}

void ComponentList::invalidateLaunchProfile()
{
	d->m_profile.reset();
}

void ComponentList::installJarMods(QStringList selectedFiles)
{
	installJarMods_internal(selectedFiles);
}

void ComponentList::installCustomJar(QString selectedFile)
{
	installCustomJar_internal(selectedFile);
}

bool ComponentList::removeComponent_internal(ComponentPtr patch)
{
	bool ok = true;
	// first, remove the patch file. this ensures it's not used anymore
	auto fileName = patch->getFilename();
	if(fileName.size())
	{
		QFile patchFile(fileName);
		if(patchFile.exists() && !patchFile.remove())
		{
			qCritical() << "File" << fileName << "could not be removed because:" << patchFile.errorString();
			return false;
		}
	}

	// FIXME: we need a generic way of removing local resources, not just jar mods...
	auto preRemoveJarMod = [&](LibraryPtr jarMod) -> bool
	{
		if (!jarMod->isLocal())
		{
			return true;
		}
		QStringList jar, temp1, temp2, temp3;
		jarMod->getApplicableFiles(currentSystem, jar, temp1, temp2, temp3, d->m_instance->jarmodsPath().absolutePath());
		QFileInfo finfo (jar[0]);
		if(finfo.exists())
		{
			QFile jarModFile(jar[0]);
			if(!jarModFile.remove())
			{
				qCritical() << "File" << jar[0] << "could not be removed because:" << jarModFile.errorString();
				return false;
			}
			return true;
		}
		return true;
	};

	auto vFile = patch->getVersionFile();
	if(vFile)
	{
		auto &jarMods = vFile->jarMods;
		for(auto &jarmod: jarMods)
		{
			ok &= preRemoveJarMod(jarmod);
		}
	}
	return ok;
}

bool ComponentList::installJarMods_internal(QStringList filepaths)
{
	QString patchDir = FS::PathCombine(d->m_instance->instanceRoot(), "patches");
	if(!FS::ensureFolderPathExists(patchDir))
	{
		return false;
	}

	if (!FS::ensureFolderPathExists(d->m_instance->jarModsDir()))
	{
		return false;
	}

	for(auto filepath:filepaths)
	{
		QFileInfo sourceInfo(filepath);
		auto uuid = QUuid::createUuid();
		QString id = uuid.toString().remove('{').remove('}');
		QString target_filename = id + ".jar";
		QString target_id = "org.multimc.jarmod." + id;
		QString target_name = sourceInfo.completeBaseName() + " (jar mod)";
		QString finalPath = FS::PathCombine(d->m_instance->jarModsDir(), target_filename);

		QFileInfo targetInfo(finalPath);
		if(targetInfo.exists())
		{
			return false;
		}

		if (!QFile::copy(sourceInfo.absoluteFilePath(),QFileInfo(finalPath).absoluteFilePath()))
		{
			return false;
		}

		auto f = std::make_shared<VersionFile>();
		auto jarMod = std::make_shared<Library>();
		jarMod->setRawName(GradleSpecifier("org.multimc.jarmods:" + id + ":1"));
		jarMod->setFilename(target_filename);
		jarMod->setDisplayName(sourceInfo.completeBaseName());
		jarMod->setHint("local");
		f->jarMods.append(jarMod);
		f->name = target_name;
		f->uid = target_id;
		QString patchFileName = FS::PathCombine(patchDir, target_id + ".json");

		QFile file(patchFileName);
		if (!file.open(QFile::WriteOnly))
		{
			qCritical() << "Error opening" << file.fileName()
						<< "for reading:" << file.errorString();
			return false;
		}
		file.write(OneSixVersionFormat::versionFileToJson(f).toJson());
		file.close();

		appendComponent(new Component(this, f->uid, f));
	}
	scheduleSave();
	invalidateLaunchProfile();
	return true;
}

bool ComponentList::installCustomJar_internal(QString filepath)
{
	QString patchDir = FS::PathCombine(d->m_instance->instanceRoot(), "patches");
	if(!FS::ensureFolderPathExists(patchDir))
	{
		return false;
	}

	QString libDir = d->m_instance->getLocalLibraryPath();
	if (!FS::ensureFolderPathExists(libDir))
	{
		return false;
	}

	auto specifier = GradleSpecifier("org.multimc:customjar:1");
	QFileInfo sourceInfo(filepath);
	QString target_filename = specifier.getFileName();
	QString target_id = specifier.artifactId();
	QString target_name = sourceInfo.completeBaseName() + " (custom jar)";
	QString finalPath = FS::PathCombine(libDir, target_filename);

	QFileInfo jarInfo(finalPath);
	if (jarInfo.exists())
	{
		if(!QFile::remove(finalPath))
		{
			return false;
		}
	}
	if (!QFile::copy(filepath, finalPath))
	{
		return false;
	}

	auto f = std::make_shared<VersionFile>();
	auto jarMod = std::make_shared<Library>();
	jarMod->setRawName(specifier);
	jarMod->setDisplayName(sourceInfo.completeBaseName());
	jarMod->setHint("local");
	f->mainJar = jarMod;
	f->name = target_name;
	f->uid = target_id;
	QString patchFileName = FS::PathCombine(patchDir, target_id + ".json");

	QFile file(patchFileName);
	if (!file.open(QFile::WriteOnly))
	{
		qCritical() << "Error opening" << file.fileName()
					<< "for reading:" << file.errorString();
		return false;
	}
	file.write(OneSixVersionFormat::versionFileToJson(f).toJson());
	file.close();

	appendComponent(new Component(this, f->uid, f));

	scheduleSave();
	invalidateLaunchProfile();
	return true;
}

std::shared_ptr<LaunchProfile> ComponentList::getProfile() const
{
	if(!d->m_profile)
	{
		try
		{
			auto profile = std::make_shared<LaunchProfile>();
			for(auto file: d->components)
			{
				qDebug() << "Applying" << file->getID() << (file->getProblemSeverity() == ProblemSeverity::Error ? "ERROR" : "GOOD");
				file->applyTo(profile.get());
			}
			d->m_profile = profile;
		}
		catch (Exception & error)
		{
			qWarning() << "Couldn't apply profile patches because: " << error.cause();
		}
	}
	return d->m_profile;
}

void ComponentList::setOldConfigVersion(const QString& uid, const QString& version)
{
	if(version.isEmpty())
	{
		return;
	}
	d->m_oldConfigVersions[uid] = version;
}

bool ComponentList::setComponentVersion(const QString& uid, const QString& version, bool important)
{
	auto iter = d->componentIndex.find(uid);
	if(iter != d->componentIndex.end())
	{
		// set existing
		(*iter)->setVersion(version);
		(*iter)->setImportant(important);
		return true;
	}
	else
	{
		// add new
		auto component = new Component(this, uid);
		component->m_version = version;
		component->m_important = important;
		appendComponent(component);
		return true;
	}
}

QString ComponentList::getComponentVersion(const QString& uid) const
{
	const auto iter = d->componentIndex.find(uid);
	if (iter != d->componentIndex.end())
	{
		return (*iter)->getVersion();
	}
	return QString();
}
