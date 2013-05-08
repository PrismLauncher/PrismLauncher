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

#include "minecraftversion.h"

MinecraftVersion::MinecraftVersion(QString descriptor, 
								   QString name, 
								   qint64 timestamp, 
								   QString dlUrl, 
								   QString etag, 
								   InstVersionList *parent) :
		InstVersion(descriptor, name, timestamp, parent), m_dlUrl(dlUrl), m_etag(etag)
{
	m_linkedVersion = NULL;
	m_isNewLauncherVersion = false;
}

MinecraftVersion::MinecraftVersion(const MinecraftVersion *linkedVersion) :
	InstVersion(linkedVersion->descriptor(), linkedVersion->name(), linkedVersion->timestamp(), 
				linkedVersion->versionList())
{
	m_linkedVersion = (MinecraftVersion *)linkedVersion;
}

MinecraftVersion::MinecraftVersion(const MinecraftVersion &other, QObject *parent) :
	InstVersion(other, parent)
{
	if (other.m_linkedVersion)
		m_linkedVersion = other.m_linkedVersion;
	else
	{
		m_dlUrl = other.downloadURL();
		m_etag = other.etag();
	}
}

QString MinecraftVersion::descriptor() const
{
	return m_descriptor;
}

QString MinecraftVersion::name() const
{
	return m_name;
}

QString MinecraftVersion::typeName() const
{
	if (m_linkedVersion)
		return m_linkedVersion->typeName();
	
	switch (versionType())
	{
	case OldSnapshot:
		return "Snapshot";
		
	case Stable:
		return "Stable";
		
	case CurrentStable:
		return "Current Stable";
		
	case Snapshot:
		return "Snapshot";
		
	case MCNostalgia:
		return "MCNostalgia";
		
	case MetaCustom:
		// Not really sure what this does, but it was in the code for v4, 
		// so it must be important... Right?
		return "Custom Meta Version";
		
	case MetaLatestSnapshot:
		return "Latest Snapshot";
		
	case MetaLatestStable:
		return "Latest Stable";
		
	default:
		return QString("Unknown Type %1").arg(versionType());
	}
}

qint64 MinecraftVersion::timestamp() const
{
	return m_timestamp;
}

bool MinecraftVersion::isForNewLauncher() const
{
	return m_isNewLauncherVersion;
}

void MinecraftVersion::setIsForNewLauncher(bool val)
{
	m_isNewLauncherVersion = val;
}

MinecraftVersion::VersionType MinecraftVersion::versionType() const
{
	return m_type;
}

void MinecraftVersion::setVersionType(MinecraftVersion::VersionType typeName)
{
	m_type = typeName;
}

QString MinecraftVersion::downloadURL() const
{
	return m_dlUrl;
}

QString MinecraftVersion::etag() const
{
	return m_etag;
}

bool MinecraftVersion::isMeta() const
{
	return versionType() == MetaCustom || 
			versionType() == MetaLatestSnapshot || 
			versionType() == MetaLatestStable;
}

InstVersion *MinecraftVersion::copyVersion(InstVersionList *newParent) const
{
	if (isMeta())
	{
		MinecraftVersion *version = new MinecraftVersion((MinecraftVersion *)m_linkedVersion);
		return version;
	}
	else
	{
		MinecraftVersion *version = new MinecraftVersion(
					descriptor(), name(), timestamp(), downloadURL(), etag(), newParent);
		version->setVersionType(versionType());
		version->setIsForNewLauncher(isForNewLauncher());
		return version;
	}
}
