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

#ifndef STDINSTVERSION_H
#define STDINSTVERSION_H

#include "instversion.h"

/*!
 * \brief Implements a standard Minecraft instance version.
 * This class also supports meta versions (i.e. versions that are essentially 
 * aliases of other versions).
 */
class StdInstVersion : public InstVersion
{
	Q_OBJECT
public:
	explicit StdInstVersion(QString descriptor, 
							QString name, 
							qint64 timestamp, 
							QString dlUrl, 
							bool hasLWJGL, 
							QString etag, 
							InstVersionList *parent);
	
	explicit StdInstVersion(StdInstVersion *linkedVersion);
	
	StdInstVersion();
	
	static StdInstVersion *mcnVersion(QString rawName, QString niceName);
	
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
	virtual QString type() const;
	virtual qint64 timestamp() const;
	virtual QString downloadURL() const;
	virtual bool hasLWJGL() const;
	virtual QString etag() const;
	
	virtual VersionType versionType() const;
	virtual void setVersionType(VersionType type);
	
	virtual bool isMeta() const;
	
	
protected:
	QString m_descriptor;
	QString m_name;
	qint64 m_timestamp;
	QString m_dlUrl;
	bool m_hasLWJGL;
	QString m_etag;
	VersionType m_type;
	
	StdInstVersion *m_linkedVersion;
};

#endif // STDINSTVERSION_H
