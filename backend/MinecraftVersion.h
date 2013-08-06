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

#include "libmmc_config.h"

#include "InstanceVersion.h"

class LIBMULTIMC_EXPORT MinecraftVersion : public InstVersion
{
	Q_OBJECT
	
public:
	explicit MinecraftVersion(QString descriptor, 
							  QString name, 
							  qint64 timestamp, 
							  QString dlUrl, 
							  QString etag, 
							  InstVersionList *parent = 0);
	
	static InstVersion *mcnVersion(QString rawName, QString niceName);
	
	enum VersionType
	{
		OldSnapshot,
		Stable,
		CurrentStable,
		Snapshot,
		Nostalgia
	};
	
	virtual QString descriptor() const;
	virtual QString name() const;
	virtual QString typeName() const;
	virtual qint64 timestamp() const;
	
	virtual VersionType versionType() const;
	virtual void setVersionType(VersionType typeName);
	
	virtual QString downloadURL() const;
	virtual QString etag() const;
	
	virtual InstVersion *copyVersion(InstVersionList *newParent) const;
	
private:
	/// The URL that this version will be downloaded from. maybe.
	QString m_dlUrl;
	
	/// ETag/MD5 Used to verify the integrity of the downloaded minecraft.jar.
	QString m_etag;
	
	/// This version's type. Used internally to identify what kind of version this is.
	VersionType m_type;
};
