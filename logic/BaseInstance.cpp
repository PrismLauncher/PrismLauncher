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

#include "BaseInstance.h"

#include <QFileInfo>
#include <QDir>

#include "settings/INISettingsObject.h"
#include "settings/Setting.h"
#include "settings/OverrideSetting.h"

#include "pathutils.h"
#include <cmdutils.h>
#include "minecraft/MinecraftVersionList.h"
#include "icons/IconList.h"

BaseInstance::BaseInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString &rootDir)
	: QObject()
{
	m_settings = settings;
	m_rootDir = rootDir;

	m_settings->registerSetting("name", "Unnamed Instance");
	m_settings->registerSetting("iconKey", "default");
	connect(ENV.icons().get(), SIGNAL(iconUpdated(QString)), SLOT(iconUpdated(QString)));
	m_settings->registerSetting("notes", "");
	m_settings->registerSetting("lastLaunchTime", 0);

	// Custom Commands
	m_settings->registerSetting({"OverrideCommands","OverrideLaunchCmd"}, false);
	m_settings->registerOverride(globalSettings->getSetting("PreLaunchCommand"));
	m_settings->registerOverride(globalSettings->getSetting("PostExitCommand"));

	// Console
	m_settings->registerSetting("OverrideConsole", false);
	m_settings->registerOverride(globalSettings->getSetting("ShowConsole"));
	m_settings->registerOverride(globalSettings->getSetting("AutoCloseConsole"));
	m_settings->registerOverride(globalSettings->getSetting("LogPrePostOutput"));
}

void BaseInstance::iconUpdated(QString key)
{
	if(iconKey() == key)
	{
		emit propertiesChanged(this);
	}
}

void BaseInstance::nuke()
{
	QDir(instanceRoot()).removeRecursively();
	emit nuked(this);
}

QString BaseInstance::id() const
{
	return QFileInfo(instanceRoot()).fileName();
}

bool BaseInstance::isRunning() const
{
	return m_isRunning;
}

void BaseInstance::setRunning(bool running)
{
	m_isRunning = running;
}

QString BaseInstance::instanceType() const
{
	return m_settings->get("InstanceType").toString();
}

QString BaseInstance::instanceRoot() const
{
	return m_rootDir;
}

InstancePtr BaseInstance::getSharedPtr()
{
	return shared_from_this();
}

SettingsObject &BaseInstance::settings() const
{
	return *m_settings;
}

BaseInstance::InstanceFlags BaseInstance::flags() const
{
	return m_flags;
}

void BaseInstance::setFlags(const InstanceFlags &flags)
{
	if (flags != m_flags)
	{
		m_flags = flags;
		emit flagsChanged();
		emit propertiesChanged(this);
	}
}

void BaseInstance::setFlag(const BaseInstance::InstanceFlag flag)
{
	m_flags |= flag;
	emit flagsChanged();
	emit propertiesChanged(this);
}

void BaseInstance::unsetFlag(const BaseInstance::InstanceFlag flag)
{
	m_flags &= ~flag;
	emit flagsChanged();
	emit propertiesChanged(this);
}

bool BaseInstance::canLaunch() const
{
	return !(flags() & VersionBrokenFlag);
}

bool BaseInstance::reload()
{
	return m_settings->reload();
}

qint64 BaseInstance::lastLaunch() const
{
	return m_settings->get("lastLaunchTime").value<qint64>();
}

void BaseInstance::setLastLaunch(qint64 val)
{
	m_settings->set("lastLaunchTime", val);
	emit propertiesChanged(this);
}

void BaseInstance::setGroupInitial(QString val)
{
	m_group = val;
	emit propertiesChanged(this);
}

void BaseInstance::setGroupPost(QString val)
{
	setGroupInitial(val);
	emit groupChanged();
}

QString BaseInstance::group() const
{
	return m_group;
}

void BaseInstance::setNotes(QString val)
{
	m_settings->set("notes", val);
}

QString BaseInstance::notes() const
{
	return m_settings->get("notes").toString();
}

void BaseInstance::setIconKey(QString val)
{
	m_settings->set("iconKey", val);
	emit propertiesChanged(this);
}

QString BaseInstance::iconKey() const
{
	return m_settings->get("iconKey").toString();
}

void BaseInstance::setName(QString val)
{
	m_settings->set("name", val);
	emit propertiesChanged(this);
}

QString BaseInstance::name() const
{
	return m_settings->get("name").toString();
}

QString BaseInstance::windowTitle() const
{
	return "MultiMC: " + name();
}

QStringList BaseInstance::extraArguments() const
{
	return Util::Commandline::splitArgs(settings().get("JvmArgs").toString());
}
