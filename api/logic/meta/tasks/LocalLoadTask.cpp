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

#include "LocalLoadTask.h"

#include <QFile>

#include "meta/format/Format.h"
#include "meta/Util.h"
#include "meta/Index.h"
#include "meta/Version.h"
#include "meta/VersionList.h"
#include "Env.h"
#include "Json.h"

namespace Meta
{
LocalLoadTask::LocalLoadTask(BaseEntity *entity, QObject *parent)
	: Task(parent), m_entity(entity)
{
}

void LocalLoadTask::executeTask()
{
	const QString fname = Meta::localDir().absoluteFilePath(filename());
	if (!QFile::exists(fname))
	{
		emitFailed(tr("File doesn't exist"));
		return;
	}

	setStatus(tr("Reading %1...").arg(name()));
	setProgress(0, 0);

	try
	{
		parse(Json::requireObject(Json::requireDocument(fname, name()), name()));
		m_entity->notifyLocalLoadComplete();
		emitSucceeded();
	}
	catch (Exception &e)
	{
		emitFailed(tr("Unable to parse file %1: %2").arg(fname, e.cause()));
	}
}


// INDEX
IndexLocalLoadTask::IndexLocalLoadTask(Index *index, QObject *parent)
	: LocalLoadTask(index, parent)
{
}
QString IndexLocalLoadTask::filename() const
{
	return "index.json";
}
QString IndexLocalLoadTask::name() const
{
	return tr("Metadata Index");
}
void IndexLocalLoadTask::parse(const QJsonObject &obj) const
{
	Format::parseIndex(obj, dynamic_cast<Index *>(entity()));
}


// VERSION LIST
VersionListLocalLoadTask::VersionListLocalLoadTask(VersionList *list, QObject *parent)
	: LocalLoadTask(list, parent)
{
}
QString VersionListLocalLoadTask::filename() const
{
	return list()->uid() + ".json";
}
QString VersionListLocalLoadTask::name() const
{
	return tr("Version List for %1").arg(list()->humanReadable());
}
void VersionListLocalLoadTask::parse(const QJsonObject &obj) const
{
	Format::parseVersionList(obj, list());
}
VersionList *VersionListLocalLoadTask::list() const
{
	return dynamic_cast<VersionList *>(entity());
}


// VERSION
VersionLocalLoadTask::VersionLocalLoadTask(Version *version, QObject *parent)
	: LocalLoadTask(version, parent)
{
}
QString VersionLocalLoadTask::filename() const
{
	return version()->uid() + "/" + version()->version() + ".json";
}
QString VersionLocalLoadTask::name() const
{
	return tr(" Version for %1").arg(version()->name());
}
void VersionLocalLoadTask::parse(const QJsonObject &obj) const
{
	Format::parseVersion(obj, version());
}
Version *VersionLocalLoadTask::version() const
{
	return dynamic_cast<Version *>(entity());
}
}
