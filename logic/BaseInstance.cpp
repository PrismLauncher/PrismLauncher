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

#include "MultiMC.h"
#include "BaseInstance.h"
#include "BaseInstance_p.h"

#include <QFileInfo>
#include <QDir>
#include "MultiMC.h"

#include "inisettingsobject.h"
#include "setting.h"
#include "overridesetting.h"

#include "pathutils.h"
#include <cmdutils.h>
#include "logic/minecraft/MinecraftVersionList.h"
#include "logic/icons/IconList.h"

BaseInstance::BaseInstance(BaseInstancePrivate *d_in, const QString &rootDir,
						   SettingsObject *settings_obj, QObject *parent)
	: QObject(parent), inst_d(d_in)
{
	I_D(BaseInstance);
	d->m_settings = std::shared_ptr<SettingsObject>(settings_obj);
	d->m_rootDir = rootDir;

	settings().registerSetting("name", "Unnamed Instance");
	settings().registerSetting("iconKey", "default");
	connect(MMC->icons().get(), SIGNAL(iconUpdated(QString)), SLOT(iconUpdated(QString)));
	settings().registerSetting("notes", "");
	settings().registerSetting("lastLaunchTime", 0);

	/*
	 * custom base jar has no default. it is determined in code... see the accessor methods for
	 *it
	 *
	 * for instances that DO NOT have the CustomBaseJar setting (legacy instances),
	 * [.]minecraft/bin/mcbackup.jar is the default base jar
	 */
	settings().registerSetting("UseCustomBaseJar", true);
	settings().registerSetting("CustomBaseJar", "");

	auto globalSettings = MMC->settings();

	// Java Settings
	settings().registerSetting("OverrideJava", false);
	settings().registerSetting("OverrideJavaLocation", false);
	settings().registerSetting("OverrideJavaArgs", false);
	settings().registerOverride(globalSettings->getSetting("JavaPath"));
	settings().registerOverride(globalSettings->getSetting("JvmArgs"));

	// Custom Commands
	settings().registerSetting({"OverrideCommands","OverrideLaunchCmd"}, false);
	settings().registerOverride(globalSettings->getSetting("PreLaunchCommand"));
	settings().registerOverride(globalSettings->getSetting("PostExitCommand"));

	// Window Size
	settings().registerSetting("OverrideWindow", false);
	settings().registerOverride(globalSettings->getSetting("LaunchMaximized"));
	settings().registerOverride(globalSettings->getSetting("MinecraftWinWidth"));
	settings().registerOverride(globalSettings->getSetting("MinecraftWinHeight"));

	// Memory
	settings().registerSetting("OverrideMemory", false);
	settings().registerOverride(globalSettings->getSetting("MinMemAlloc"));
	settings().registerOverride(globalSettings->getSetting("MaxMemAlloc"));
	settings().registerOverride(globalSettings->getSetting("PermGen"));

	// Console
	settings().registerSetting("OverrideConsole", false);
	settings().registerOverride(globalSettings->getSetting("ShowConsole"));
	settings().registerOverride(globalSettings->getSetting("AutoCloseConsole"));
	settings().registerOverride(globalSettings->getSetting("LogPrePostOutput"));
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

QString BaseInstance::instanceType() const
{
	I_D(BaseInstance);
	return d->m_settings->get("InstanceType").toString();
}

QString BaseInstance::instanceRoot() const
{
	I_D(BaseInstance);
	return d->m_rootDir;
}

QString BaseInstance::minecraftRoot() const
{
	QFileInfo mcDir(PathCombine(instanceRoot(), "minecraft"));
	QFileInfo dotMCDir(PathCombine(instanceRoot(), ".minecraft"));

	if (dotMCDir.exists() && !mcDir.exists())
		return dotMCDir.filePath();
	else
		return mcDir.filePath();
}

InstanceList *BaseInstance::instList() const
{
	if (parent()->inherits("InstanceList"))
		return (InstanceList *)parent();
	else
		return NULL;
}

std::shared_ptr<BaseVersionList> BaseInstance::versionList() const
{
	return MMC->minecraftlist();
}

SettingsObject &BaseInstance::settings() const
{
	I_D(BaseInstance);
	return *d->m_settings;
}

QSet<BaseInstance::InstanceFlag> BaseInstance::flags() const
{
	I_D(const BaseInstance);
	return QSet<InstanceFlag>(d->m_flags);
}

void BaseInstance::setFlags(const QSet<InstanceFlag> &flags)
{
	I_D(BaseInstance);
	if (flags != d->m_flags)
	{
		d->m_flags = flags;
		emit flagsChanged();
		emit propertiesChanged(this);
	}
}

bool BaseInstance::canLaunch() const
{
	return !flags().contains(VersionBrokenFlag);
}

bool BaseInstance::reload()
{
	return settings().reload();
}

QString BaseInstance::baseJar() const
{
	I_D(BaseInstance);
	bool customJar = d->m_settings->get("UseCustomBaseJar").toBool();
	if (customJar)
	{
		return customBaseJar();
	}
	else
		return defaultBaseJar();
}

QString BaseInstance::customBaseJar() const
{
	I_D(BaseInstance);
	QString value = d->m_settings->get("CustomBaseJar").toString();
	if (value.isNull() || value.isEmpty())
	{
		return defaultCustomBaseJar();
	}
	return value;
}

void BaseInstance::setCustomBaseJar(QString val)
{
	I_D(BaseInstance);
	if (val.isNull() || val.isEmpty() || val == defaultCustomBaseJar())
		d->m_settings->reset("CustomBaseJar");
	else
		d->m_settings->set("CustomBaseJar", val);
}

void BaseInstance::setShouldUseCustomBaseJar(bool val)
{
	I_D(BaseInstance);
	d->m_settings->set("UseCustomBaseJar", val);
}

bool BaseInstance::shouldUseCustomBaseJar() const
{
	I_D(BaseInstance);
	return d->m_settings->get("UseCustomBaseJar").toBool();
}

qint64 BaseInstance::lastLaunch() const
{
	I_D(BaseInstance);
	return d->m_settings->get("lastLaunchTime").value<qint64>();
}
void BaseInstance::setLastLaunch(qint64 val)
{
	I_D(BaseInstance);
	d->m_settings->set("lastLaunchTime", val);
	emit propertiesChanged(this);
}

void BaseInstance::setGroupInitial(QString val)
{
	I_D(BaseInstance);
	d->m_group = val;
	emit propertiesChanged(this);
}

void BaseInstance::setGroupPost(QString val)
{
	setGroupInitial(val);
	emit groupChanged();
}

QString BaseInstance::group() const
{
	I_D(BaseInstance);
	return d->m_group;
}

void BaseInstance::setNotes(QString val)
{
	I_D(BaseInstance);
	d->m_settings->set("notes", val);
}
QString BaseInstance::notes() const
{
	I_D(BaseInstance);
	return d->m_settings->get("notes").toString();
}

void BaseInstance::setIconKey(QString val)
{
	I_D(BaseInstance);
	d->m_settings->set("iconKey", val);
	emit propertiesChanged(this);
}
QString BaseInstance::iconKey() const
{
	I_D(BaseInstance);
	return d->m_settings->get("iconKey").toString();
}

void BaseInstance::setName(QString val)
{
	I_D(BaseInstance);
	d->m_settings->set("name", val);
	emit propertiesChanged(this);
}

QString BaseInstance::name() const
{
	I_D(BaseInstance);
	return d->m_settings->get("name").toString();
}

QString BaseInstance::windowTitle() const
{
	return "MultiMC: " + name();
}

QStringList BaseInstance::extraArguments() const
{
	return Util::Commandline::splitArgs(settings().get("JvmArgs").toString());
}
