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

#include "instancelist.h"

#include "data/siglist_impl.h"

#include <QDir>
#include <QFile>
#include <QDirIterator>

#include "instance.h"
#include "instanceloader.h"

#include "util/pathutils.h"


InstanceList::InstanceList(const QString &instDir, QObject *parent) :
	QObject(parent), m_instDir(instDir)
{
	
}

InstanceList::InstListError InstanceList::loadList()
{
	QDir dir(m_instDir);
	QDirIterator iter(dir);
	
	while (iter.hasNext())
	{
		QString subDir = iter.next();
		if (QFileInfo(PathCombine(subDir, "instance.cfg")).exists())
		{
			QSharedPointer<Instance> inst;
			InstanceLoader::InstTypeError error = InstanceLoader::loader.
					loadInstance(inst.data(), subDir);
			
			if (inst.data() && error == InstanceLoader::NoError)
			{
				qDebug(QString("Loaded instance %1").arg(inst->name()).toUtf8());
				inst->setParent(this);
				append(QSharedPointer<Instance>(inst));
			}
			else if (error != InstanceLoader::NotAnInstance)
			{
				QString errorMsg = QString("Failed to load instance %1: ").
						arg(QFileInfo(subDir).baseName()).toUtf8();
				
				switch (error)
				{
				case InstanceLoader::TypeNotRegistered:
					errorMsg += "Instance type not found.";
					break;
				}
				qDebug(errorMsg.toUtf8());
			}
			else if (!inst.data())
			{
				qDebug(QString("Error loading instance %1. Instance loader returned null.").
					   arg(QFileInfo(subDir).baseName()).toUtf8());
			}
		}
	}
	
	return NoError;
}
