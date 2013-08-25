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

#include "logic/lists/InstanceList.h"
#include "logic/BaseInstance.h"
#include "logic/InstanceFactory.h"

#include "pathutils.h"

const static int GROUP_FILE_FORMAT_VERSION = 1;

InstanceList::InstanceList(const QString &instDir, QObject *parent) :
	QObject(parent), m_instDir("instances")
{
	
}

InstanceList::~InstanceList()
{
	saveGroupList();
}


void InstanceList::groupChanged()
{
	// save the groups. save all of them.
	saveGroupList();
}

void InstanceList::saveGroupList()
{
	QString groupFileName = m_instDir + "/instgroups.json";
	QFile groupFile(groupFileName);
	
	// if you can't open the file, fail
	if (!groupFile.open(QIODevice::WriteOnly| QIODevice::Truncate))
	{
		// An error occurred. Ignore it.
		qDebug("Failed to read instance group file.");
		return;
	}
	QTextStream out(&groupFile);
	QMap<QString, QSet<QString> > groupMap;
	for(auto instance: m_instances)
	{
		QString id = instance->id();
		QString group = instance->group();
		if(group.isEmpty())
			continue;
		if(!groupMap.count(group))
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
	toplevel.insert("formatVersion",QJsonValue(QString("1")));
	QJsonObject groupsArr;
	for(auto iter = groupMap.begin(); iter != groupMap.end(); iter++)
	{
		auto list = iter.value();
		auto name = iter.key();
		QJsonObject groupObj;
		QJsonArray instanceArr;
		groupObj.insert("hidden",QJsonValue(QString("false")));
		for(auto item: list)
		{
			instanceArr.append(QJsonValue(item));
		}
		groupObj.insert("instances",instanceArr);
		groupsArr.insert(name,groupObj);
	}
	toplevel.insert("groups",groupsArr);
	QJsonDocument doc(toplevel);
	groupFile.write(doc.toJson(QJsonDocument::Indented));
	groupFile.close();
}

void InstanceList::loadGroupList(QMap<QString, QString> & groupMap)
{
	QString groupFileName = m_instDir + "/instgroups.json";
	
	// if there's no group file, fail
	if(!QFileInfo(groupFileName).exists())
		return;
	
	QFile groupFile(groupFileName);
	
	// if you can't open the file, fail
	if (!groupFile.open(QIODevice::ReadOnly))
	{
		// An error occurred. Ignore it.
		qDebug("Failed to read instance group file.");
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
		qWarning(QString("Failed to parse instance group file: %1 at offset %2").
					arg(error.errorString(), QString::number(error.offset)).toUtf8());
		return;
	}
	
	// if the root of the json wasn't an object, fail
	if (!jsonDoc.isObject())
	{
		qWarning("Invalid group file. Root entry should be an object.");
		return;
	}
	
	QJsonObject rootObj = jsonDoc.object();
	
	// Make sure the format version matches, otherwise fail.
	if (rootObj.value("formatVersion").toVariant().toInt() != GROUP_FILE_FORMAT_VERSION)
		return;
	
	// Get the groups. if it's not an object, fail
	if (!rootObj.value("groups").isObject())
	{
		qWarning("Invalid group list JSON: 'groups' should be an object.");
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
			qWarning(QString("Group '%1' in the group list should "
								"be an object.").arg(groupName).toUtf8());
			continue;
		}
		
		QJsonObject groupObj = iter.value().toObject();
		if (!groupObj.value("instances").isArray())
		{
			qWarning(QString("Group '%1' in the group list is invalid. "
								"It should contain an array "
								"called 'instances'.").arg(groupName).toUtf8());
			continue;
		}
		
		// Iterate through the list of instances in the group.
		QJsonArray instancesArray = groupObj.value("instances").toArray();
		
		for (QJsonArray::iterator iter2 = instancesArray.begin(); 
				iter2 != instancesArray.end(); iter2++)
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
	
	m_instances.clear();
	QDir dir(m_instDir);
	QDirIterator iter(dir);
	while (iter.hasNext())
	{
		QString subDir = iter.next();
		if (!QFileInfo(PathCombine(subDir, "instance.cfg")).exists())
			continue;
		
		BaseInstance *instPtr = NULL;
		auto &loader = InstanceFactory::get();
		auto error = loader.loadInstance(instPtr, subDir);
		
		switch(error)
		{
			case InstanceFactory::NoLoadError:
				break;
			case InstanceFactory::NotAnInstance:
				break;
		}
		
		if (error != InstanceFactory::NoLoadError &&
			error != InstanceFactory::NotAnInstance)
		{
			QString errorMsg = QString("Failed to load instance %1: ").
					arg(QFileInfo(subDir).baseName()).toUtf8();
			
			switch (error)
			{
			default:
				errorMsg += QString("Unknown instance loader error %1").
						arg(error);
				break;
			}
			qDebug(errorMsg.toUtf8());
		}
		else if (!instPtr)
		{
			qDebug(QString("Error loading instance %1. Instance loader returned null.").
					arg(QFileInfo(subDir).baseName()).toUtf8());
		}
		else
		{
			QSharedPointer<BaseInstance> inst(instPtr);
			auto iter = groupMap.find(inst->id());
			if(iter != groupMap.end())
			{
				inst->setGroupInitial((*iter));
			}
			qDebug(QString("Loaded instance %1").arg(inst->name()).toUtf8());
			inst->setParent(this);
			m_instances.append(inst);
			connect(instPtr, SIGNAL(propertiesChanged(BaseInstance*)),this, SLOT(propertiesChanged(BaseInstance*)));
			connect(instPtr, SIGNAL(groupChanged()),this, SLOT(groupChanged()));
		}
	}
	emit invalidated();
	return NoError;
}

/// Clear all instances. Triggers notifications.
void InstanceList::clear()
{
	saveGroupList();
	m_instances.clear();
	emit invalidated();
};

/// Add an instance. Triggers notifications, returns the new index
int InstanceList::add(InstancePtr t)
{
	m_instances.append(t);
	emit instanceAdded(count() - 1);
	return count() - 1;
}

InstancePtr InstanceList::getInstanceById(QString instId)
{
	QListIterator<InstancePtr> iter(m_instances);
	InstancePtr inst;
	while(iter.hasNext())
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

void InstanceList::propertiesChanged(BaseInstance * inst)
{
	for(int i = 0; i < m_instances.count(); i++)
	{
		if(inst == m_instances[i].data())
		{
			emit instanceChanged(i);
			break;
		}
	}
}
