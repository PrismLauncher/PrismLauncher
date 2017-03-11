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

#include "tasks/LocalLoadTask.h"
#include "tasks/RemoteLoadTask.h"
#include "format/Format.h"

namespace Meta
{
Version::Version(const QString &uid, const QString &version)
	: BaseVersion(), m_uid(uid), m_version(version)
{
}

QString Version::descriptor()
{
	return m_version;
}
QString Version::name()
{
	return m_version;
}
QString Version::typeString() const
{
	return m_type;
}

QDateTime Version::time() const
{
	return QDateTime::fromMSecsSinceEpoch(m_time * 1000, Qt::UTC);
}

std::unique_ptr<Task> Version::remoteUpdateTask()
{
	return std::unique_ptr<VersionRemoteLoadTask>(new VersionRemoteLoadTask(this, this));
}
std::unique_ptr<Task> Version::localUpdateTask()
{
	return std::unique_ptr<VersionLocalLoadTask>(new VersionLocalLoadTask(this, this));
}

void Version::merge(const std::shared_ptr<BaseEntity> &other)
{
	VersionPtr version = std::dynamic_pointer_cast<Version>(other);
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

	setData(version->m_data);
}

QString Version::localFilename() const
{
	return m_uid + '/' + m_version + ".json";
}
QJsonObject Version::serialized() const
{
	return Format::serializeVersion(this);
}

void Version::setType(const QString &type)
{
	m_type = type;
	emit typeChanged();
}
void Version::setTime(const qint64 time)
{
	m_time = time;
	emit timeChanged();
}
void Version::setRequires(const QVector<Reference> &requires)
{
	m_requires = requires;
	emit requiresChanged();
}
void Version::setData(const VersionFilePtr &data)
{
	m_data = data;
}
}
