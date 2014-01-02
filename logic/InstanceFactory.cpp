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

#include "InstanceFactory.h"

#include <QDir>
#include <QFileInfo>

#include "BaseInstance.h"
#include "LegacyInstance.h"
#include "LegacyFTBInstance.h"
#include "OneSixInstance.h"
#include "OneSixFTBInstance.h"
#include "NostalgiaInstance.h"
#include "BaseVersion.h"
#include "MinecraftVersion.h"

#include "inifile.h"
#include <inisettingsobject.h>
#include <setting.h>

#include "pathutils.h"
#include "logger/QsLog.h"

InstanceFactory InstanceFactory::loader;

InstanceFactory::InstanceFactory() : QObject(NULL)
{
}

InstanceFactory::InstLoadError InstanceFactory::loadInstance(BaseInstance *&inst,
															 const QString &instDir)
{
	auto m_settings = new INISettingsObject(PathCombine(instDir, "instance.cfg"));

	m_settings->registerSetting("InstanceType", "Legacy");

	QString inst_type = m_settings->get("InstanceType").toString();

	// FIXME: replace with a map lookup, where instance classes register their types
	if (inst_type == "Legacy")
	{
		inst = new LegacyInstance(instDir, m_settings, this);
	}
	else if (inst_type == "OneSix")
	{
		inst = new OneSixInstance(instDir, m_settings, this);
	}
	else if (inst_type == "Nostalgia")
	{
		inst = new NostalgiaInstance(instDir, m_settings, this);
	}
	else if (inst_type == "LegacyFTB")
	{
		inst = new LegacyFTBInstance(instDir, m_settings, this);
	}
	else if (inst_type == "OneSixFTB")
	{
		inst = new OneSixFTBInstance(instDir, m_settings, this);
	}
	else
	{
		return InstanceFactory::UnknownLoadError;
	}
	return NoLoadError;
}

InstanceFactory::InstCreateError InstanceFactory::createInstance(BaseInstance *&inst,
																 BaseVersionPtr version,
																 const QString &instDir,
																 const InstType type)
{
	QDir rootDir(instDir);

	QLOG_DEBUG() << instDir.toUtf8();
	if (!rootDir.exists() && !rootDir.mkpath("."))
	{
		return InstanceFactory::CantCreateDir;
	}
	auto mcVer = std::dynamic_pointer_cast<MinecraftVersion>(version);
	if (!mcVer)
		return InstanceFactory::NoSuchVersion;

	auto m_settings = new INISettingsObject(PathCombine(instDir, "instance.cfg"));
	m_settings->registerSetting("InstanceType", "Legacy");

	if (type == NormalInst)
	{
		switch (mcVer->type)
		{
		case MinecraftVersion::Legacy:
			m_settings->set("InstanceType", "Legacy");
			inst = new LegacyInstance(instDir, m_settings, this);
			inst->setIntendedVersionId(version->descriptor());
			inst->setShouldUseCustomBaseJar(false);
			break;
		case MinecraftVersion::OneSix:
			m_settings->set("InstanceType", "OneSix");
			inst = new OneSixInstance(instDir, m_settings, this);
			inst->setIntendedVersionId(version->descriptor());
			inst->setShouldUseCustomBaseJar(false);
			break;
		case MinecraftVersion::Nostalgia:
			m_settings->set("InstanceType", "Nostalgia");
			inst = new NostalgiaInstance(instDir, m_settings, this);
			inst->setIntendedVersionId(version->descriptor());
			inst->setShouldUseCustomBaseJar(false);
			break;
		default:
		{
			delete m_settings;
			return InstanceFactory::NoSuchVersion;
		}
		}
	}
	else if (type == FTBInstance)
	{
		switch (mcVer->type)
		{
		case MinecraftVersion::Legacy:
			m_settings->set("InstanceType", "LegacyFTB");
			inst = new LegacyFTBInstance(instDir, m_settings, this);
			inst->setIntendedVersionId(version->descriptor());
			inst->setShouldUseCustomBaseJar(false);
			break;
		case MinecraftVersion::OneSix:
			m_settings->set("InstanceType", "OneSixFTB");
			inst = new OneSixFTBInstance(instDir, m_settings, this);
			inst->setIntendedVersionId(version->descriptor());
			inst->setShouldUseCustomBaseJar(false);
			break;
		default:
		{
			delete m_settings;
			return InstanceFactory::NoSuchVersion;
		}
		}
	}
	else
	{
		delete m_settings;
		return InstanceFactory::NoSuchVersion;
	}

	// FIXME: really, how do you even know?
	return InstanceFactory::NoCreateError;
}

InstanceFactory::InstCreateError InstanceFactory::copyInstance(BaseInstance *&newInstance,
															   BaseInstance *&oldInstance,
															   const QString &instDir)
{
	QDir rootDir(instDir);

	QLOG_DEBUG() << instDir.toUtf8();
	if (!copyPath(oldInstance->instanceRoot(), instDir))
	{
		rootDir.removeRecursively();
		return InstanceFactory::CantCreateDir;
	}
	auto m_settings = new INISettingsObject(PathCombine(instDir, "instance.cfg"));
	m_settings->registerSetting("InstanceType", "Legacy");
	QString inst_type = m_settings->get("InstanceType").toString();

	if(inst_type == "OneSixFTB")
		m_settings->set("InstanceType", "OneSix");
	if(inst_type == "LegacyFTB")
		m_settings->set("InstanceType", "Legacy");

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
