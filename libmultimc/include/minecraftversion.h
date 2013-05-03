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

#ifndef MINECRAFTVERSION_H
#define MINECRAFTVERSION_H

#include "libmmc_config.h"

#include "instversion.h"

class LIBMULTIMC_EXPORT MinecraftVersion : public InstVersion
{
	Q_OBJECT
	
	/*!
	 * This version's type. Used internally to identify what kind of version this is.
	 */
	Q_PROPERTY(VersionType versionType READ versionType WRITE setVersionType)
	
	/*!
	 * The URL that this version will be downloaded from.
	 */
	Q_PROPERTY(QString downloadURL READ downloadURL)
	
	/*!
	 * ETag/MD5 Used to verify the integrity of the downloaded minecraft.jar.
	 */
	Q_PROPERTY(QString etag READ etag)
	
public:
	explicit MinecraftVersion(QString descriptor, 
							  QString name, 
							  qint64 timestamp, 
							  QString dlUrl, 
							  QString etag, 
							  InstVersionList *parent = 0);
	
	/*!
	 * Creates a meta version that links to the given version.
	 * This is *NOT* a copy constructor.
	 * \param linkedVersion the version that the meta version will link to.
	 */
	explicit MinecraftVersion(const MinecraftVersion *linkedVersion);
	
	MinecraftVersion(const MinecraftVersion &other, QObject *parent);
	
	static InstVersion *mcnVersion(QString rawName, QString niceName);
	
	enum VersionType
	{
		OldSnapshot,
		Stable,
		CurrentStable,
		Snapshot,
		MCNostalgia,
		MetaCustom,
		MetaLatestSnapshot,
		MetaLatestStable
	};
	
	virtual QString descriptor() const;
	virtual QString name() const;
	virtual QString typeName() const;
	virtual qint64 timestamp() const;
	
	virtual VersionType versionType() const;
	virtual void setVersionType(VersionType typeName);
	
	virtual QString downloadURL() const;
	virtual QString etag() const;
	virtual bool isMeta() const;
	
	virtual InstVersion *copyVersion(InstVersionList *newParent) const;
	
private:
	InstVersion *m_linkedVersion;
	
	QString m_dlUrl;
	QString m_etag;
	VersionType m_type;
};

#endif // MINECRAFTVERSION_H
