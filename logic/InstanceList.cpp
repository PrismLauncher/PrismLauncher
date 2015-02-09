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
#include <QDebug>

#include "InstanceList.h"
#include "icons/IconList.h"
#include "BaseInstance.h"

//FIXME: this really doesn't belong *here*
#include "minecraft/OneSixInstance.h"
#include "minecraft/LegacyInstance.h"
#include "minecraft/MinecraftVersion.h"
#include "settings/INISettingsObject.h"
#include "ftb/FTBPlugin.h"

const static int GROUP_FILE_FORMAT_VERSION = 1;

InstanceList::InstanceList(SettingsObjectPtr globalSettings, const QString &instDir, QObject *parent)
	: QAbstractListModel(parent), m_instDir(instDir)
{
	m_globalSettings = globalSettings;
	if (!QDir::current().exists(m_instDir))
	{
		QDir::current().mkpath(m_instDir);
	}
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
	case InstanceIDRole:
    {
        return pdata->id();
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
		return ENV.icons()->getIcon(key);
	}
	// HACK: see GroupView.h in gui!
	case GroupRole:
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
		qCritical() << "Failed to save instance group file.";
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
		qCritical() << "Failed to read instance group file.";
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
		qCritical() << QString("Failed to parse instance group file: %1 at offset %2")
							.arg(error.errorString(), QString::number(error.offset))
							.toUtf8();
		return;
	}

	// if the root of the json wasn't an object, fail
	if (!jsonDoc.isObject())
	{
		qWarning() << "Invalid group file. Root entry should be an object.";
		return;
	}

	QJsonObject rootObj = jsonDoc.object();

	// Make sure the format version matches, otherwise fail.
	if (rootObj.value("formatVersion").toVariant().toInt() != GROUP_FILE_FORMAT_VERSION)
		return;

	// Get the groups. if it's not an object, fail
	if (!rootObj.value("groups").isObject())
	{
		qWarning() << "Invalid group list JSON: 'groups' should be an object.";
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
			qWarning() << QString("Group '%1' in the group list should "
								   "be an object.")
							   .arg(groupName)
							   .toUtf8();
			continue;
		}

		QJsonObject groupObj = iter.value().toObject();
		if (!groupObj.value("instances").isArray())
		{
			qWarning() << QString("Group '%1' in the group list is invalid. "
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
			qDebug() << "Loading MultiMC instance from " << subDir;
			InstancePtr instPtr;
			auto error = loadInstance(instPtr, subDir);
			if(!continueProcessInstance(instPtr, error, subDir, groupMap))
				continue;
			tempList.append(instPtr);
		}
	}

	// FIXME: generalize
	FTBPlugin::loadInstances(m_globalSettings, groupMap, tempList);

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

bool InstanceList::continueProcessInstance(InstancePtr instPtr, const int error,
										   const QDir &dir, QMap<QString, QString> &groupMap)
{
	if (error != InstanceList::NoLoadError && error != InstanceList::NotAnInstance)
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
		qCritical() << errorMsg.toUtf8();
		return false;
	}
	else if (!instPtr)
	{
		qCritical() << QString("Error loading instance %1. Instance loader returned null.")
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
		qDebug() << "Loaded instance " << instPtr->name() << " from " << dir.absolutePath();
		return true;
	}
}

InstanceList::InstLoadError
InstanceList::loadInstance(InstancePtr &inst, const QString &instDir)
{
	auto instanceSettings = std::make_shared<INISettingsObject>(PathCombine(instDir, "instance.cfg"));

	instanceSettings->registerSetting("InstanceType", "Legacy");

	QString inst_type = instanceSettings->get("InstanceType").toString();

	// FIXME: replace with a map lookup, where instance classes register their types
	if (inst_type == "OneSix" || inst_type == "Nostalgia")
	{
		inst.reset(new OneSixInstance(m_globalSettings, instanceSettings, instDir));
	}
	else if (inst_type == "Legacy")
	{
		inst.reset(new LegacyInstance(m_globalSettings, instanceSettings, instDir));
	}
	else
	{
		return InstanceList::UnknownLoadError;
	}
	inst->init();
	return NoLoadError;
}

InstanceList::InstCreateError
InstanceList::createInstance(InstancePtr &inst, BaseVersionPtr version, const QString &instDir)
{
	QDir rootDir(instDir);

	qDebug() << instDir.toUtf8();
	if (!rootDir.exists() && !rootDir.mkpath("."))
	{
		qCritical() << "Can't create instance folder" << instDir;
		return InstanceList::CantCreateDir;
	}

	if (!version)
	{
		qCritical() << "Can't create instance for non-existing MC version";
		return InstanceList::NoSuchVersion;
	}

	auto instanceSettings = std::make_shared<INISettingsObject>(PathCombine(instDir, "instance.cfg"));
	instanceSettings->registerSetting("InstanceType", "Legacy");

	auto minecraftVersion = std::dynamic_pointer_cast<MinecraftVersion>(version);
	if(minecraftVersion)
	{
		auto mcVer = std::dynamic_pointer_cast<MinecraftVersion>(version);
		instanceSettings->set("InstanceType", "OneSix");
		inst.reset(new OneSixInstance(m_globalSettings, instanceSettings, instDir));
		inst->setIntendedVersionId(version->descriptor());
		inst->init();
		return InstanceList::NoCreateError;
	}
	return InstanceList::NoSuchVersion;
}

InstanceList::InstCreateError
InstanceList::copyInstance(InstancePtr &newInstance, InstancePtr &oldInstance, const QString &instDir)
{
	QDir rootDir(instDir);

	qDebug() << instDir.toUtf8();
	if (!copyPath(oldInstance->instanceRoot(), instDir))
	{
		rootDir.removeRecursively();
		return InstanceList::CantCreateDir;
	}

	INISettingsObject settings_obj(PathCombine(instDir, "instance.cfg"));
	settings_obj.registerSetting("InstanceType", "Legacy");
	QString inst_type = settings_obj.get("InstanceType").toString();

	oldInstance->copy(instDir);

	auto error = loadInstance(newInstance, instDir);

	switch (error)
	{
	case NoLoadError:
		return NoCreateError;
	case NotAnInstance:
		rootDir.removeRecursively();
		return CantCreateDir;
	default:
	case UnknownLoadError:
		rootDir.removeRecursively();
		return UnknownCreateError;
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
