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
	const QString fname = Meta::localDir().absoluteFilePath(m_entity->localFilename());
	if (!QFile::exists(fname))
	{
		emitFailed(tr("File doesn't exist"));
		return;
	}
	setStatus(tr("Reading %1...").arg(fname));
	setProgress(0, 0);

	try
	{
		m_entity->parse(Json::requireObject(Json::requireDocument(fname, fname), fname));
		m_entity->notifyLocalLoadComplete();
		emitSucceeded();
	}
	catch (Exception &e)
	{
		emitFailed(tr("Unable to parse file %1: %2").arg(fname, e.cause()));
	}
}
}
