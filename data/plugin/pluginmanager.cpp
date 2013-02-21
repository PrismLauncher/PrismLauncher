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
#include <QVariant>

#include <QJsonObject>

#include <QtPlugin>

#include "instancetypeinterface.h"

// MultiMC's API version. This must match the "api" field in each plugin's 
// metadata or MultiMC won't consider them valid MultiMC plugin.
#define MMC_API_VERSION "MultiMC5-API-1"

PluginManager PluginManager::manager;

PluginManager::PluginManager() :
	QObject(NULL)
{
	
}

bool PluginManager::loadPlugins(QString pluginDir)
{
	// Delete the loaded plugins and clear the list.
	for (int i = 0; i < m_plugins.count(); i++)
	{
		delete m_plugins[i];
	}
	m_plugins.clear();
	
	qDebug(QString("Loading plugins from directory: %1").
		   arg(pluginDir).toUtf8());
	
	QDir dir(pluginDir);
	QDirIterator iter(dir);
	
	while (iter.hasNext())
	{
		QFileInfo pluginFile(dir.absoluteFilePath(iter.next()));
		
		if (pluginFile.exists() && pluginFile.isFile())
		{
			qDebug(QString("Attempting to load plugin: %1").
				   arg(pluginFile.canonicalFilePath()).toUtf8());
			
			QPluginLoader *pluginLoader = new QPluginLoader(pluginFile.absoluteFilePath());
			
			QJsonObject pluginInfo = pluginLoader->metaData();
			QJsonObject pluginMetadata = pluginInfo.value("MetaData").toObject();
			
			if (pluginMetadata.value("api").toString("") != MMC_API_VERSION)
			{
				// If "api" is not specified, it's not a MultiMC plugin.
				qDebug(QString("Not loading plugin %1. Not a valid MultiMC plugin. "
							   "API: %2").
					   arg(pluginFile.canonicalFilePath(), pluginMetadata.value("api").toString("")).toUtf8());
				continue;
			}
			
			qDebug(QString("Loaded plugin: %1").
				   arg(pluginInfo.value("IID").toString()).toUtf8());
			m_plugins.push_back(pluginLoader);
		}
	}
	
	return true;
}

QPluginLoader *PluginManager::getPlugin(int index)
{
	return m_plugins[index];
}

void PluginManager::initInstanceTypes()
{
	for (int i = 0; i < m_plugins.count(); i++)
	{
		InstanceTypeInterface *instType = qobject_cast<InstanceTypeInterface *>(m_plugins[i]->instance());
		
		if (instType)
		{
			// TODO: Handle errors
			InstanceLoader::get().registerInstanceType(instType);
		}
	}
}
