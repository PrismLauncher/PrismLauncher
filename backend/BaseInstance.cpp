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

#include "BaseInstance.h"
#include "BaseInstance_p.h"

#include <QFileInfo>

#include "inisettingsobject.h"
#include "setting.h"
#include "overridesetting.h"

#include "pathutils.h"
#include <lists/MinecraftVersionList.h>


BaseInstance::BaseInstance( BaseInstancePrivate* d_in,
							const QString& rootDir,
							SettingsObject* settings_obj,
							QObject* parent
						  )
:inst_d(d_in), QObject(parent)
{
	I_D(BaseInstance);
	d->m_settings = settings_obj;
	d->m_rootDir = rootDir;
	
	settings().registerSetting(new Setting("name", "Unnamed Instance"));
	settings().registerSetting(new Setting("iconKey", "default"));
	settings().registerSetting(new Setting("notes", ""));
	settings().registerSetting(new Setting("lastLaunchTime", 0));
	
	// Java Settings
	settings().registerSetting(new Setting("OverrideJava", false));
	settings().registerSetting(new OverrideSetting("JavaPath", globalSettings->getSetting("JavaPath")));
	settings().registerSetting(new OverrideSetting("JvmArgs", globalSettings->getSetting("JvmArgs")));
	
	// Custom Commands
	settings().registerSetting(new Setting("OverrideCommands", false));
	settings().registerSetting(new OverrideSetting("PreLaunchCommand", globalSettings->getSetting("PreLaunchCommand")));
	settings().registerSetting(new OverrideSetting("PostExitCommand", globalSettings->getSetting("PostExitCommand")));
	
	// Window Size
	settings().registerSetting(new Setting("OverrideWindow", false));
	settings().registerSetting(new OverrideSetting("LaunchMaximized", globalSettings->getSetting("LaunchMaximized")));
	settings().registerSetting(new OverrideSetting("MinecraftWinWidth", globalSettings->getSetting("MinecraftWinWidth")));
	settings().registerSetting(new OverrideSetting("MinecraftWinHeight", globalSettings->getSetting("MinecraftWinHeight")));
	
	// Memory
	settings().registerSetting(new Setting("OverrideMemory", false));
	settings().registerSetting(new OverrideSetting("MinMemAlloc", globalSettings->getSetting("MinMemAlloc")));
	settings().registerSetting(new OverrideSetting("MaxMemAlloc", globalSettings->getSetting("MaxMemAlloc")));
	
	// Auto login
	settings().registerSetting(new Setting("OverrideLogin", false));
	settings().registerSetting(new OverrideSetting("AutoLogin", globalSettings->getSetting("AutoLogin")));
	
	// Console
	settings().registerSetting(new Setting("OverrideConsole", false));
	settings().registerSetting(new OverrideSetting("ShowConsole", globalSettings->getSetting("ShowConsole")));
	settings().registerSetting(new OverrideSetting("AutoCloseConsole", globalSettings->getSetting("AutoCloseConsole")));
}

QString BaseInstance::id() const
{
	return QFileInfo(rootDir()).fileName();
}

QString BaseInstance::instanceType() const
{
	I_D(BaseInstance);
	return d->m_settings->get("InstanceType").toString();
}


QString BaseInstance::rootDir() const
{
	I_D(BaseInstance);
	return d->m_rootDir;
}

InstanceList *BaseInstance::instList() const
{
	if (parent()->inherits("InstanceList"))
		return (InstanceList *)parent();
	else
		return NULL;
}

InstVersionList *BaseInstance::versionList() const
{
	return &MinecraftVersionList::getMainList();
}

SettingsObject &BaseInstance::settings() const
{
	I_D(BaseInstance);
	return *d->m_settings;
}

qint64 BaseInstance::lastLaunch() const
{
	I_D(BaseInstance);
	return d->m_settings->get ( "lastLaunchTime" ).value<qint64>();
}
void BaseInstance::setLastLaunch ( qint64 val )
{
	I_D(BaseInstance);
	d->m_settings->set ( "lastLaunchTime", val );
	emit propertiesChanged ( this );
}

void BaseInstance::setGroup ( QString val )
{
	I_D(BaseInstance);
	d->m_group = val;
	emit propertiesChanged ( this );
}
QString BaseInstance::group() const
{
	I_D(BaseInstance);
	return d->m_group;
}

void BaseInstance::setNotes ( QString val )
{
	I_D(BaseInstance);
	d->m_settings->set ( "notes", val );
}
QString BaseInstance::notes() const
{
	I_D(BaseInstance);
	return d->m_settings->get ( "notes" ).toString();
}

void BaseInstance::setIconKey ( QString val )
{
	I_D(BaseInstance);
	d->m_settings->set ( "iconKey", val );
	emit propertiesChanged ( this );
}
QString BaseInstance::iconKey() const
{
	I_D(BaseInstance);
	return d->m_settings->get ( "iconKey" ).toString();
}

void BaseInstance::setName ( QString val )
{
	I_D(BaseInstance);
	d->m_settings->set ( "name", val );
	emit propertiesChanged ( this );
}
QString BaseInstance::name() const
{
	I_D(BaseInstance);
	return d->m_settings->get ( "name" ).toString();
}
