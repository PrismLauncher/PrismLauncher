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

#include "instancebase.h"

#include <QFileInfo>
#include <QDir>

#include "../util/pathutils.h"

InstanceBase::InstanceBase(QString dir, QObject *parent) :
	QObject(parent), 
	rootDir(dir)
{
	QFileInfo cfgFile(PathCombine(rootDir, "instance.cfg"));
	
	if (cfgFile.exists())
	{
		if(!config.loadFile(cfgFile.absoluteFilePath()))
		{
			QString debugmsg("Can't load instance config file for instance ");
			debugmsg+= getInstID();
			qDebug(debugmsg.toLocal8Bit());
		}
	}
	else
	{
		QString debugmsg("Can't find instance config file for instance ");
		debugmsg+= getInstID();
		debugmsg += " : ";
		debugmsg += 
		debugmsg+=" ... is this an instance even?";
		qDebug(debugmsg.toLocal8Bit());
	}
	currentGroup = nullptr;
}

QString InstanceBase::getRootDir() const
{
	return rootDir;
}


///////////// Config Values /////////////

// Name
QString InstanceBase::getInstName() const
{
	return config.get("name", "Unnamed").toString();
}

void InstanceBase::setInstName(QString name)
{
	config.set("name", name);
}

QString InstanceBase::getInstID() const
{
	return QDir(rootDir).dirName();
}

InstanceModelItem* InstanceBase::getParent() const
{
	return currentGroup;
}

QVariant InstanceBase::data ( int role ) const
{
	switch(role)
	{
		case Qt::DisplayRole:
			return getInstName();
		default:
			return QVariant();
	}
}
int InstanceBase::getRow() const
{
	return currentGroup->getIndexOf((InstanceBase*)this);
}

InstanceModelItem* InstanceBase::getChild ( int index ) const
{
	return nullptr;
}
InstanceModel* InstanceBase::getModel() const
{
	return currentGroup->getModel();
}
IMI_type InstanceBase::getModelItemType() const
{
	return IMI_Instance;
}
int InstanceBase::numChildren() const
{
	return 0;
}
