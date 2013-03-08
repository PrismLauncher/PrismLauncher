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

#include "stdinstversion.h"

StdInstVersion::StdInstVersion(QString descriptor, 
							   QString name, 
							   qint64 timestamp, 
							   QString dlUrl, 
							   bool hasLWJGL, 
							   QString etag, 
							   InstVersionList *parent) :
	InstVersion(parent), m_descriptor(descriptor), m_name(name), m_timestamp(timestamp),
	m_dlUrl(dlUrl), m_hasLWJGL(hasLWJGL), m_etag(etag)
{
	m_linkedVersion = NULL;
}

StdInstVersion::StdInstVersion(StdInstVersion *linkedVersion)
{
	m_linkedVersion = linkedVersion;
}

StdInstVersion::StdInstVersion()
{
	m_timestamp = 0;
	m_hasLWJGL = false;
	m_linkedVersion = NULL;
}

StdInstVersion *StdInstVersion::mcnVersion(QString rawName, QString niceName)
{
	StdInstVersion *version = new StdInstVersion;
	version->m_descriptor = rawName;
	version->m_name = niceName;
	version->setVersionType(MCNostalgia);
	return version;
}

QString StdInstVersion::descriptor() const
{
	if (m_linkedVersion)
		return m_linkedVersion->descriptor();
	return m_descriptor;
}

QString StdInstVersion::name() const
{
	if (m_linkedVersion)
		return m_linkedVersion->name();
	return m_name;
}

QString StdInstVersion::type() const
{
	if (m_linkedVersion)
		return m_linkedVersion->type();
	
	switch (versionType())
	{
	case OldSnapshot:
		return "Old Snapshot";
		
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

qint64 StdInstVersion::timestamp() const
{
	if (m_linkedVersion)
		return m_linkedVersion->timestamp();
	return m_timestamp;
}

QString StdInstVersion::downloadURL() const
{
	if (m_linkedVersion)
		return m_linkedVersion->downloadURL();
	return m_dlUrl;
}

bool StdInstVersion::hasLWJGL() const
{
	if (m_linkedVersion)
		return m_linkedVersion->hasLWJGL();
	return m_hasLWJGL;
}

QString StdInstVersion::etag() const
{
	if (m_linkedVersion)
		return m_linkedVersion->etag();
	return m_etag;
}

StdInstVersion::VersionType StdInstVersion::versionType() const
{
	return m_type;
}

void StdInstVersion::setVersionType(StdInstVersion::VersionType type)
{
	m_type = type;
}

bool StdInstVersion::isMeta() const
{
	return versionType() == MetaCustom || 
			versionType() == MetaLatestSnapshot || 
			versionType() == MetaLatestStable;
}
