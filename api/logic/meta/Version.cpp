/* Copyright 2015-2017 MultiMC Contributors
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

#include "Version.h"

#include <QDateTime>

#include "JsonFormat.h"
#include "minecraft/MinecraftProfile.h"

Meta::Version::Version(const QString &uid, const QString &version)
	: BaseVersion(), m_uid(uid), m_version(version)
{
}

Meta::Version::~Version()
{
}

QString Meta::Version::descriptor()
{
	return m_version;
}
QString Meta::Version::name()
{
	if(m_data)
		return m_data->name;
	return m_uid;
}
QString Meta::Version::typeString() const
{
	return m_type;
}

QDateTime Meta::Version::time() const
{
	return QDateTime::fromMSecsSinceEpoch(m_time * 1000, Qt::UTC);
}

void Meta::Version::parse(const QJsonObject& obj)
{
	parseVersion(obj, this);
}

void Meta::Version::merge(const std::shared_ptr<BaseEntity> &other)
{
	VersionPtr version = std::dynamic_pointer_cast<Version>(other);
	if(version->m_providesRecommendations)
	{
		if(m_recommended != version->m_recommended)
		{
			setRecommended(version->m_recommended);
		}
	}
	if (m_type != version->m_type)
	{
		setType(version->m_type);
	}
	if (m_time != version->m_time)
	{
		setTime(version->m_time);
	}
	if (m_requires != version->m_requires)
	{
		setRequires(version->m_requires);
	}
	if (m_parentUid != version->m_parentUid)
	{
		setParentUid(version->m_parentUid);
	}
	if(version->m_data)
	{
		setData(version->m_data);
	}
}

QString Meta::Version::localFilename() const
{
	return m_uid + '/' + m_version + ".json";
}

void Meta::Version::setParentUid(const QString& parentUid)
{
	m_parentUid = parentUid;
	emit requiresChanged();
}

void Meta::Version::setType(const QString &type)
{
	m_type = type;
	emit typeChanged();
}

void Meta::Version::setTime(const qint64 time)
{
	m_time = time;
	emit timeChanged();
}

void Meta::Version::setRequires(const QHash<QString, QString> &requires)
{
	m_requires = requires;
	emit requiresChanged();
}

void Meta::Version::setData(const VersionFilePtr &data)
{
	m_data = data;
}

void Meta::Version::setProvidesRecommendations()
{
	m_providesRecommendations = true;
}

void Meta::Version::setRecommended(bool recommended)
{
	m_recommended = recommended;
}
