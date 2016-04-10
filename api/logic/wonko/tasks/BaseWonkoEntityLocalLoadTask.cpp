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

#include "BaseWonkoEntityLocalLoadTask.h"

#include <QFile>

#include "wonko/format/WonkoFormat.h"
#include "wonko/WonkoUtil.h"
#include "wonko/WonkoIndex.h"
#include "wonko/WonkoVersion.h"
#include "wonko/WonkoVersionList.h"
#include "Env.h"
#include "Json.h"

BaseWonkoEntityLocalLoadTask::BaseWonkoEntityLocalLoadTask(BaseWonkoEntity *entity, QObject *parent)
	: Task(parent), m_entity(entity)
{
}

void BaseWonkoEntityLocalLoadTask::executeTask()
{
	const QString fname = Wonko::localWonkoDir().absoluteFilePath(filename());
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

//      WONKO INDEX      //
WonkoIndexLocalLoadTask::WonkoIndexLocalLoadTask(WonkoIndex *index, QObject *parent)
	: BaseWonkoEntityLocalLoadTask(index, parent)
{
}
QString WonkoIndexLocalLoadTask::filename() const
{
	return "index.json";
}
QString WonkoIndexLocalLoadTask::name() const
{
	return tr("Wonko Index");
}
void WonkoIndexLocalLoadTask::parse(const QJsonObject &obj) const
{
	WonkoFormat::parseIndex(obj, dynamic_cast<WonkoIndex *>(entity()));
}

//      WONKO VERSION LIST      //
WonkoVersionListLocalLoadTask::WonkoVersionListLocalLoadTask(WonkoVersionList *list, QObject *parent)
	: BaseWonkoEntityLocalLoadTask(list, parent)
{
}
QString WonkoVersionListLocalLoadTask::filename() const
{
	return list()->uid() + ".json";
}
QString WonkoVersionListLocalLoadTask::name() const
{
	return tr("Wonko Version List for %1").arg(list()->humanReadable());
}
void WonkoVersionListLocalLoadTask::parse(const QJsonObject &obj) const
{
	WonkoFormat::parseVersionList(obj, list());
}
WonkoVersionList *WonkoVersionListLocalLoadTask::list() const
{
	return dynamic_cast<WonkoVersionList *>(entity());
}

//      WONKO VERSION      //
WonkoVersionLocalLoadTask::WonkoVersionLocalLoadTask(WonkoVersion *version, QObject *parent)
	: BaseWonkoEntityLocalLoadTask(version, parent)
{
}
QString WonkoVersionLocalLoadTask::filename() const
{
	return version()->uid() + "/" + version()->version() + ".json";
}
QString WonkoVersionLocalLoadTask::name() const
{
	return tr("Wonko Version for %1").arg(version()->name());
}
void WonkoVersionLocalLoadTask::parse(const QJsonObject &obj) const
{
	WonkoFormat::parseVersion(obj, version());
}
WonkoVersion *WonkoVersionLocalLoadTask::version() const
{
	return dynamic_cast<WonkoVersion *>(entity());
}
