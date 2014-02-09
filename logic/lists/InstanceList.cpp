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

#include <QDir>
#include <QSet>
#include <QFile>
#include <QDirIterator>
#include <QThread>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QXmlStreamReader>
#include <QRegularExpression>
#include <pathutils.h>

#include "MultiMC.h"
#include "logic/lists/InstanceList.h"
#include "logic/icons/IconList.h"
#include "logic/lists/MinecraftVersionList.h"
#include "logic/BaseInstance.h"
#include "logic/InstanceFactory.h"
#include "logger/QsLog.h"
#include <gui/groupview/GroupView.h>

const static int GROUP_FILE_FORMAT_VERSION = 1;

InstanceList::InstanceList(const QString &instDir, QObject *parent)
	: QAbstractListModel(parent), m_instDir(instDir)
{
	connect(MMC, &MultiMC::aboutToQuit, this, &InstanceList::saveGroupList);

	if (!QDir::current().exists(m_instDir))
	{
		QDir::current().mkpath(m_instDir);
	}

	/*
	 * FIXME HACK: instances sometimes need to be created at launch. They need the versions for
	 * that.
	 *
	 * Remove this. it has no business of reloading the whole list. The instances which
	 * need it should track such events themselves and CHANGE THEIR DATA ONLY!
	 */
	connect(MMC->minecraftlist().get(), &MinecraftVersionList::modelReset, this,
			&InstanceList::loadList);
}

InstanceList::~InstanceList()
{
}

int InstanceList::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return m_instances.count();
}

QModelIndex InstanceList::index(int row, int column, const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	if (row < 0 || row >= m_instances.size())
		return QModelIndex();
	return createIndex(row, column, (void *)m_instances.at(row).get());
}

QVariant InstanceList::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
	{
		return QVariant();
	}
	BaseInstance *pdata = static_cast<BaseInstance *>(index.internalPointer());
	switch (role)
	{
	case InstancePointerRole:
	{
		QVariant v = qVariantFromValue((void *)pdata);
		return v;
	}
	case Qt::DisplayRole:
	{
		return pdata->name();
	}
	case Qt::ToolTipRole:
	{
		return pdata->instanceRoot();
	}
	case Qt::DecorationRole:
	{
		QString key = pdata->iconKey();
		return MMC->icons()->getIcon(key);
	}
	// for now.
	case GroupViewRoles::GroupRole:
	{
		return pdata->group();
	}
	default:
		break;
	}
	return QVariant();
}

Qt::ItemFlags InstanceList::flags(const QModelIndex &index) const
{
	Qt::ItemFlags f;
	if (index.isValid())
	{
		f |= (Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	}
	return f;
}

void InstanceList::groupChanged()
{
	// save the groups. save all of them.
	saveGroupList();
}

QStringList InstanceList::getGroups()
{
	return m_groups.toList();
}

void InstanceList::saveGroupList()
{
	QString groupFileName = m_instDir + "/instgroups.json";
	QFile groupFile(groupFileName);

	// if you can't open the file, fail
	if (!groupFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		// An error occurred. Ignore it.
		QLOG_ERROR() << "Failed to save instance group file.";
		return;
	}
	QTextStream out(&groupFile);
	QMap<QString, QSet<QString>> groupMap;
	for (auto instance : m_instances)
	{
		QString id = instance->id();
		QString group = instance->group();
		if (group.isEmpty())
			continue;

		// keep a list/set of groups for choosing
		m_groups.insert(group);

		if (!groupMap.count(group))
		{
			QSet<QString> set;
			set.insert(id);
			groupMap[group] = set;
		}
		else
		{
			QSet<QString> &set = groupMap[group];
			set.insert(id);
		}
	}
	QJsonObject toplevel;
	toplevel.insert("formatVersion", QJsonValue(QString("1")));
	QJsonObject groupsArr;
	for (auto iter = groupMap.begin(); iter != groupMap.end(); iter++)
	{
		auto list = iter.value();
		auto name = iter.key();
		QJsonObject groupObj;
		QJsonArray instanceArr;
		groupObj.insert("hidden", QJsonValue(QString("false")));
		for (auto item : list)
		{
			instanceArr.append(QJsonValue(item));
		}
		groupObj.insert("instances", instanceArr);
		groupsArr.insert(name, groupObj);
	}
	toplevel.insert("groups", groupsArr);
	QJsonDocument doc(toplevel);
	groupFile.write(doc.toJson());
	groupFile.close();
}

void InstanceList::loadGroupList(QMap<QString, QString> &groupMap)
{
	QString groupFileName = m_instDir + "/instgroups.json";

	// if there's no group file, fail
	if (!QFileInfo(groupFileName).exists())
		return;

	QFile groupFile(groupFileName);

	// if you can't open the file, fail
	if (!groupFile.open(QIODevice::ReadOnly))
	{
		// An error occurred. Ignore it.
		QLOG_ERROR() << "Failed to read instance group file.";
		return;
	}

	QTextStream in(&groupFile);
	QString jsonStr = in.readAll();
	groupFile.close();

	QJsonParseError error;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonStr.toUtf8(), &error);

	// if the json was bad, fail
	if (error.error != QJsonParseError::NoError)
	{
		QLOG_ERROR() << QString("Failed to parse instance group file: %1 at offset %2")
							.arg(error.errorString(), QString::number(error.offset))
							.toUtf8();
		return;
	}

	// if the root of the json wasn't an object, fail
	if (!jsonDoc.isObject())
	{
		QLOG_WARN() << "Invalid group file. Root entry should be an object.";
		return;
	}

	QJsonObject rootObj = jsonDoc.object();

	// Make sure the format version matches, otherwise fail.
	if (rootObj.value("formatVersion").toVariant().toInt() != GROUP_FILE_FORMAT_VERSION)
		return;

	// Get the groups. if it's not an object, fail
	if (!rootObj.value("groups").isObject())
	{
		QLOG_WARN() << "Invalid group list JSON: 'groups' should be an object.";
		return;
	}

	// Iterate through all the groups.
	QJsonObject groupMapping = rootObj.value("groups").toObject();
	for (QJsonObject::iterator iter = groupMapping.begin(); iter != groupMapping.end(); iter++)
	{
		QString groupName = iter.key();

		// If not an object, complain and skip to the next one.
		if (!iter.value().isObject())
		{
			QLOG_WARN() << QString("Group '%1' in the group list should "
								   "be an object.")
							   .arg(groupName)
							   .toUtf8();
			continue;
		}

		QJsonObject groupObj = iter.value().toObject();
		if (!groupObj.value("instances").isArray())
		{
			QLOG_WARN() << QString("Group '%1' in the group list is invalid. "
								   "It should contain an array "
								   "called 'instances'.")
							   .arg(groupName)
							   .toUtf8();
			continue;
		}

		// keep a list/set of groups for choosing
		m_groups.insert(groupName);

		// Iterate through the list of instances in the group.
		QJsonArray instancesArray = groupObj.value("instances").toArray();

		for (QJsonArray::iterator iter2 = instancesArray.begin(); iter2 != instancesArray.end();
			 iter2++)
		{
			groupMap[(*iter2).toString()] = groupName;
		}
	}
}

QList<FTBRecord> InstanceList::discoverFTBInstances()
{
	QList<FTBRecord> records;
	QDir dir = QDir(MMC->settings()->get("FTBLauncherRoot").toString());
	QDir dataDir = QDir(MMC->settings()->get("FTBRoot").toString());
	if (!dir.exists())
	{
		QLOG_INFO() << "The FTB launcher directory specified does not exist. Please check your "
					   "settings.";
		return records;
	}
	else if (!dataDir.exists())
	{
		QLOG_INFO() << "The FTB directory specified does not exist. Please check your settings";
		return records;
	}
	dir.cd("ModPacks");
	auto allFiles = dir.entryList(QDir::Readable | QDir::Files, QDir::Name);
	for (auto filename : allFiles)
	{
		if (!filename.endsWith(".xml"))
			continue;
		auto fpath = dir.absoluteFilePath(filename);
		QFile f(fpath);
		QLOG_INFO() << "Discovering FTB instances -- " << fpath;
		if (!f.open(QFile::ReadOnly))
			continue;

		// read the FTB packs XML.
		QXmlStreamReader reader(&f);
		while (!reader.atEnd())
		{
			switch (reader.readNext())
			{
			case QXmlStreamReader::StartElement:
			{
				if (reader.name() == "modpack")
				{
					QXmlStreamAttributes attrs = reader.attributes();
					FTBRecord record;
					record.dirName = attrs.value("dir").toString();
					record.instanceDir = dataDir.absoluteFilePath(record.dirName);
					record.templateDir = dir.absoluteFilePath(record.dirName);
					QDir test(record.instanceDir);
					if (!test.exists())
						continue;
					record.name = attrs.value("name").toString();
					if(record.name.contains("voxel", Qt::CaseInsensitive))
						continue;
					record.logo = attrs.value("logo").toString();
					record.mcVersion = attrs.value("mcVersion").toString();
					record.description = attrs.value("description").toString();
					records.append(record);
				}
				break;
			}
			case QXmlStreamReader::EndElement:
				break;
			case QXmlStreamReader::Characters:
				break;
			default:
				break;
			}
		}
		f.close();
	}
	return records;
}

void InstanceList::loadFTBInstances(QMap<QString, QString> &groupMap,
									QList<InstancePtr> &tempList)
{
	auto records = discoverFTBInstances();
	if (!records.size())
	{
		QLOG_INFO() << "No FTB instances to load.";
		return;
	}
	QLOG_INFO() << "Loading FTB instances! -- got " << records.size();
	// process the records we acquired.
	for (auto record : records)
	{
		QLOG_INFO() << "Loading FTB instance from " << record.instanceDir;
		QString iconKey = record.logo;
		iconKey.remove(QRegularExpression("\\..*"));
		MMC->icons()->addIcon(iconKey, iconKey, PathCombine(record.templateDir, record.logo),
							  MMCIcon::Transient);

		if (!QFileInfo(PathCombine(record.instanceDir, "instance.cfg")).exists())
		{
			QLOG_INFO() << "Converting " << record.name << " as new.";
			BaseInstance *instPtr = NULL;
			auto &factory = InstanceFactory::get();
			auto version = MMC->minecraftlist()->findVersion(record.mcVersion);
			if (!version)
			{
				QLOG_ERROR() << "Can't load instance " << record.instanceDir
							 << " because minecraft version " << record.mcVersion
							 << " can't be resolved.";
				continue;
			}
			auto error = factory.createInstance(instPtr, version, record.instanceDir,
												InstanceFactory::FTBInstance);

			if (!instPtr || error != InstanceFactory::NoCreateError)
				continue;

			instPtr->setGroupInitial("FTB");
			instPtr->setName(record.name);
			instPtr->setIconKey(iconKey);
			instPtr->setIntendedVersionId(record.mcVersion);
			instPtr->setNotes(record.description);
			if(!continueProcessInstance(instPtr, error, record.instanceDir, groupMap))
				continue;
			tempList.append(InstancePtr(instPtr));
		}
		else
		{
			QLOG_INFO() << "Loading existing " << record.name;
			BaseInstance *instPtr = NULL;
			auto error = InstanceFactory::get().loadInstance(instPtr, record.instanceDir);
			if (!instPtr || error != InstanceFactory::NoCreateError)
				continue;
			instPtr->setGroupInitial("FTB");
			instPtr->setName(record.name);
			instPtr->setIconKey(iconKey);
			if (instPtr->intendedVersionId() != record.mcVersion)
				instPtr->setIntendedVersionId(record.mcVersion);
			instPtr->setNotes(record.description);
			if(!continueProcessInstance(instPtr, error, record.instanceDir, groupMap))
				continue;
			tempList.append(InstancePtr(instPtr));
		}
	}
}

InstanceList::InstListError InstanceList::loadList()
{
	// load the instance groups
	QMap<QString, QString> groupMap;
	loadGroupList(groupMap);

	QList<InstancePtr> tempList;
	{
		QDirIterator iter(m_instDir, QDir::Dirs | QDir::NoDot | QDir::NoDotDot | QDir::Readable,
						  QDirIterator::FollowSymlinks);
		while (iter.hasNext())
		{
			QString subDir = iter.next();
			if (!QFileInfo(PathCombine(subDir, "instance.cfg")).exists())
				continue;
			QLOG_INFO() << "Loading MultiMC instance from " << subDir;
			BaseInstance *instPtr = NULL;
			auto error = InstanceFactory::get().loadInstance(instPtr, subDir);
			if(!continueProcessInstance(instPtr, error, subDir, groupMap))
				continue;
			tempList.append(InstancePtr(instPtr));
		}
	}

	if (MMC->settings()->get("TrackFTBInstances").toBool())
	{
		loadFTBInstances(groupMap, tempList);
	}
	beginResetModel();
	m_instances.clear();
	for(auto inst: tempList)
	{
		inst->setParent(this);
		connect(inst.get(), SIGNAL(propertiesChanged(BaseInstance *)), this,
				SLOT(propertiesChanged(BaseInstance *)));
		connect(inst.get(), SIGNAL(groupChanged()), this, SLOT(groupChanged()));
		connect(inst.get(), SIGNAL(nuked(BaseInstance *)), this,
				SLOT(instanceNuked(BaseInstance *)));
		m_instances.append(inst);
	}
	endResetModel();
	emit dataIsInvalid();
	return NoError;
}

/// Clear all instances. Triggers notifications.
void InstanceList::clear()
{
	beginResetModel();
	saveGroupList();
	m_instances.clear();
	endResetModel();
	emit dataIsInvalid();
}

void InstanceList::on_InstFolderChanged(const Setting &setting, QVariant value)
{
	m_instDir = value.toString();
	loadList();
}

/// Add an instance. Triggers notifications, returns the new index
int InstanceList::add(InstancePtr t)
{
	beginInsertRows(QModelIndex(), m_instances.size(), m_instances.size());
	m_instances.append(t);
	t->setParent(this);
	connect(t.get(), SIGNAL(propertiesChanged(BaseInstance *)), this,
			SLOT(propertiesChanged(BaseInstance *)));
	connect(t.get(), SIGNAL(groupChanged()), this, SLOT(groupChanged()));
	connect(t.get(), SIGNAL(nuked(BaseInstance *)), this, SLOT(instanceNuked(BaseInstance *)));
	endInsertRows();
	return count() - 1;
}

InstancePtr InstanceList::getInstanceById(QString instId) const
{
	if (m_instances.isEmpty())
	{
		return InstancePtr();
	}

	QListIterator<InstancePtr> iter(m_instances);
	InstancePtr inst;
	while (iter.hasNext())
	{
		inst = iter.next();
		if (inst->id() == instId)
			break;
	}
	if (inst->id() != instId)
		return InstancePtr();
	else
		return iter.peekPrevious();
}

QModelIndex InstanceList::getInstanceIndexById(const QString &id) const
{
	return index(getInstIndex(getInstanceById(id).get()));
}

int InstanceList::getInstIndex(BaseInstance *inst) const
{
	for (int i = 0; i < m_instances.count(); i++)
	{
		if (inst == m_instances[i].get())
		{
			return i;
		}
	}
	return -1;
}

bool InstanceList::continueProcessInstance(BaseInstance *instPtr, const int error,
										   const QDir &dir, QMap<QString, QString> &groupMap)
{
	if (error != InstanceFactory::NoLoadError && error != InstanceFactory::NotAnInstance)
	{
		QString errorMsg = QString("Failed to load instance %1: ")
							   .arg(QFileInfo(dir.absolutePath()).baseName())
							   .toUtf8();

		switch (error)
		{
		default:
			errorMsg += QString("Unknown instance loader error %1").arg(error);
			break;
		}
		QLOG_ERROR() << errorMsg.toUtf8();
		return false;
	}
	else if (!instPtr)
	{
		QLOG_ERROR() << QString("Error loading instance %1. Instance loader returned null.")
							.arg(QFileInfo(dir.absolutePath()).baseName())
							.toUtf8();
		return false;
	}
	else
	{
		auto iter = groupMap.find(instPtr->id());
		if (iter != groupMap.end())
		{
			instPtr->setGroupInitial((*iter));
		}
		QLOG_INFO() << "Loaded instance " << instPtr->name() << " from " << dir.absolutePath();
		return true;
	}
}

void InstanceList::instanceNuked(BaseInstance *inst)
{
	int i = getInstIndex(inst);
	if (i != -1)
	{
		beginRemoveRows(QModelIndex(), i, i);
		m_instances.removeAt(i);
		endRemoveRows();
	}
}

void InstanceList::propertiesChanged(BaseInstance *inst)
{
	int i = getInstIndex(inst);
	if (i != -1)
	{
		emit dataChanged(index(i), index(i));
	}
}

InstanceProxyModel::InstanceProxyModel(QObject *parent) : GroupedProxyModel(parent)
{
}

bool InstanceProxyModel::subSortLessThan(const QModelIndex &left,
										 const QModelIndex &right) const
{
	BaseInstance *pdataLeft = static_cast<BaseInstance *>(left.internalPointer());
	BaseInstance *pdataRight = static_cast<BaseInstance *>(right.internalPointer());
	QString sortMode = MMC->settings()->get("InstSortMode").toString();
	if (sortMode == "LastLaunch")
	{
		return pdataLeft->lastLaunch() > pdataRight->lastLaunch();
	}
	else
	{
		return QString::localeAwareCompare(pdataLeft->name(), pdataRight->name()) < 0;
	}
}
