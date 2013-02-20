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

#include "pluginmanager.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

#include <QtPlugin>

#include "data/plugin/instancetypeplugin.h"

PluginManager PluginManager::manager;

PluginManager::PluginManager() :
	QObject(NULL)
{
	
}

bool PluginManager::loadPlugins(QString pluginDir)
{
	m_plugins.clear();
	
	QDir dir(pluginDir);
	QDirIterator iter(dir);
	
	while (iter.hasNext())
	{
		QFileInfo pluginFile(dir.absoluteFilePath(iter.next()));
		
		if (pluginFile.exists() && pluginFile.isFile())
		{
			QPluginLoader pluginLoader(pluginFile.absoluteFilePath());
			pluginLoader.load();
			QObject *plugin = pluginLoader.instance();
			if (plugin)
			{
				qDebug(QString("Loaded plugin %1.").
					   arg(pluginFile.baseName()).toUtf8());
				m_plugins.push_back(plugin);
			}
			else
			{
				qWarning(QString("Error loading plugin %1. Not a valid plugin.").
						 arg(pluginFile.baseName()).toUtf8());
			}
		}
	}
	
	return true;
}

bool PluginManager::initInstanceTypes()
{
	for (int i = 0; i < m_plugins.count(); i++)
	{
		InstanceTypePlugin *plugin = qobject_cast<InstanceTypePlugin *>(m_plugins[i]);
		if (plugin)
		{
			QList<InstanceType *> instanceTypes = plugin->getInstanceTypes();
			
			for (int i = 0; i < instanceTypes.count(); i++)
			{
				InstanceLoader::InstTypeError error = 
						InstanceLoader::loader.registerInstanceType(instanceTypes[i]);
				switch (error)
				{
				case InstanceLoader::TypeIDExists:
					qWarning(QString("Instance type %1 already registered.").
							 arg(instanceTypes[i]->typeID()).toUtf8());
				}
			}
		}
	}
	
	return true;
}

QObject *PluginManager::getPlugin(int index)
{
	return m_plugins[index];
}
