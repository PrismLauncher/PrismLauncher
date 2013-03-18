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

#include "include/instancelist.h"

#include <QDir>
#include <QFile>
#include <QDirIterator>
#include <QThread>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "include/instance.h"
#include "include/instanceloader.h"

#include "pathutils.h"

const static int GROUP_FILE_FORMAT_VERSION = 1;

InstanceList::InstanceList(const QString &instDir, QObject *parent) :
	QObject(parent), m_instDir("instances")
{
	
}

InstanceList::InstListError InstanceList::loadList()
{
	QDir dir(m_instDir);
	QDirIterator iter(dir);
	
	QString groupFileName = m_instDir + "/instgroups.json";
	// temporary map from instance ID to group name
	QMap<QString, QString> groupMap;
	
	// HACK: this is really an if. breaks after one iteration.
	while (QFileInfo(groupFileName).exists())
	{
		QFile groupFile(groupFileName);
		
		if (!groupFile.open(QIODevice::ReadOnly))
		{
			// An error occurred. Ignore it.
			qDebug("Failed to read instance group file.");
			break;
		}
		
		QTextStream in(&groupFile);
		QString jsonStr = in.readAll();
		groupFile.close();
		
		QJsonParseError error;
		QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonStr.toUtf8(), &error);
		
		if (error.error != QJsonParseError::NoError)
		{
			qWarning(QString("Failed to parse instance group file: %1 at offset %2").
					 arg(error.errorString(), QString::number(error.offset)).toUtf8());
			break;
		}
		
		if (!jsonDoc.isObject())
		{
			qWarning("Invalid group file. Root entry should be an object.");
			break;
		}
		
		QJsonObject rootObj = jsonDoc.object();
		
		// Make sure the format version matches.
		if (rootObj.value("formatVersion").toVariant().toInt() == GROUP_FILE_FORMAT_VERSION)
		{
			// Get the group list.
			if (!rootObj.value("groups").isObject())
			{
				qWarning("Invalid group list JSON: 'groups' should be an object.");
				break;
			}
			
			// Iterate through the list.
			QJsonObject groupList = rootObj.value("groups").toObject();
			
			for (QJsonObject::iterator iter = groupList.begin(); 
				 iter != groupList.end(); iter++)
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
				/*
				// Create the group object.
				InstanceGroup *group = new InstanceGroup(groupName, this);
				groups.push_back(group);
				
				// If 'hidden' isn't a bool value, just assume it's false.
				if (groupObj.value("hidden").isBool() && groupObj.value("hidden").toBool())
				{
					group->setHidden(groupObj.value("hidden").toBool());
				}
				*/
				
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
		break;
	}
	m_instances.clear();
	while (iter.hasNext())
	{
		QString subDir = iter.next();
		if (QFileInfo(PathCombine(subDir, "instance.cfg")).exists())
		{
			Instance *instPtr = NULL;
			
			InstanceLoader::InstTypeError error = InstanceLoader::get().
					loadInstance(instPtr, subDir);
			
			if (error != InstanceLoader::NoError &&
					 error != InstanceLoader::NotAnInstance)
			{
				QString errorMsg = QString("Failed to load instance %1: ").
						arg(QFileInfo(subDir).baseName()).toUtf8();
				
				switch (error)
				{
				case InstanceLoader::TypeNotRegistered:
					errorMsg += "Instance type not found.";
					break;
					
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
				QSharedPointer<Instance> inst(instPtr);
				auto iter = groupMap.find(inst->id());
				if(iter != groupMap.end())
				{
					inst->setGroup((*iter));
				}
				qDebug(QString("Loaded instance %1").arg(inst->name()).toUtf8());
				inst->setParent(this);
				m_instances.append(inst);
			}
		}
	}
	emit invalidated();
	return NoError;
}

/// Clear all instances. Triggers notifications.
void InstanceList::clear()
{
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