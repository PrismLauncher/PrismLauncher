/* Copyright 2015 MultiMC Contributors
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

#include "WonkoVersion.h"

#include <QDateTime>

#include "tasks/BaseWonkoEntityLocalLoadTask.h"
#include "tasks/BaseWonkoEntityRemoteLoadTask.h"
#include "format/WonkoFormat.h"

WonkoVersion::WonkoVersion(const QString &uid, const QString &version)
	: BaseVersion(), m_uid(uid), m_version(version)
{
}

QString WonkoVersion::descriptor()
{
	return m_version;
}
QString WonkoVersion::name()
{
	return m_version;
}
QString WonkoVersion::typeString() const
{
	return m_type;
}

QDateTime WonkoVersion::time() const
{
	return QDateTime::fromMSecsSinceEpoch(m_time * 1000, Qt::UTC);
}

std::unique_ptr<Task> WonkoVersion::remoteUpdateTask()
{
	return std::unique_ptr<WonkoVersionRemoteLoadTask>(new WonkoVersionRemoteLoadTask(this, this));
}
std::unique_ptr<Task> WonkoVersion::localUpdateTask()
{
	return std::unique_ptr<WonkoVersionLocalLoadTask>(new WonkoVersionLocalLoadTask(this, this));
}

void WonkoVersion::merge(const std::shared_ptr<BaseWonkoEntity> &other)
{
	WonkoVersionPtr version = std::dynamic_pointer_cast<WonkoVersion>(other);
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

QString WonkoVersion::localFilename() const
{
	return m_uid + '/' + m_version + ".json";
}
QJsonObject WonkoVersion::serialized() const
{
	return WonkoFormat::serializeVersion(this);
}

void WonkoVersion::setType(const QString &type)
{
	m_type = type;
	emit typeChanged();
}
void WonkoVersion::setTime(const qint64 time)
{
	m_time = time;
	emit timeChanged();
}
void WonkoVersion::setRequires(const QVector<WonkoReference> &requires)
{
	m_requires = requires;
	emit requiresChanged();
}
void WonkoVersion::setData(const VersionFilePtr &data)
{
	m_data = data;
}
