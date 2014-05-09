/* Copyright 2013 Andrew Okin
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

#pragma once

#include "logic/BaseVersion.h"
#include "VersionPatch.h"
#include <QStringList>
#include <QSet>

struct MinecraftVersion : public BaseVersion, public VersionPatch
{
	/// The version's timestamp - this is primarily used for sorting versions in a list.
	qint64 timestamp;

	/// The URL that this version will be downloaded from. maybe.
	QString download_url;

	/// is this the latest version?
	bool is_latest = false;

	/// is this a snapshot?
	bool is_snapshot = false;

	/// is this a built-in version that comes with MultiMC?
	bool is_builtin = false;
	
	/// the human readable version name
	QString m_name;

	/// the version ID.
	QString m_descriptor;

	/// version traits. generally launcher business...
	QSet<QString> m_traits;

	/// The main class this version uses (if any, can be empty).
	QString m_mainClass;

	/// The applet class this version uses (if any, can be empty).
	QString m_appletClass;

	bool usesLegacyLauncher()
	{
		return m_traits.contains("legacyLaunch") || m_traits.contains("aplhaLaunch");
	}
	
	virtual QString descriptor() override
	{
		return m_descriptor;
	}

	virtual QString name() override
	{
		return m_name;
	}

	virtual QString typeString() const override
	{
		if (is_latest && is_snapshot)
		{
			return QObject::tr("Latest snapshot");
		}
		else if(is_latest)
		{
			return QObject::tr("Latest release");
		}
		else if(is_snapshot)
		{
			return QObject::tr("Snapshot");
		}
		else if(is_builtin)
		{
			return QObject::tr("Museum piece");
		}
		else
		{
			return QObject::tr("Regular release");
		}
	}
	
	virtual bool hasJarMods() override
	{
		return false;
	}
	
	virtual bool isVanilla() override
	{
		return true;
	}
	
	virtual void applyTo(VersionFinal *version)
	{
		// umm... what now?
	}
};
