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

#pragma once

#include "tasks/Task.h"
#include <memory>

namespace Net
{
class Download;
}

namespace Meta
{
class BaseEntity;
class Index;
class VersionList;
class Version;

// FIXME: this is now just an oddly constructed NetJob, get rid of it.
class RemoteLoadTask : public Task
{
	Q_OBJECT
public:
	explicit RemoteLoadTask(BaseEntity *entity, QObject *parent = nullptr);

private:
	void executeTask() override;

	BaseEntity *m_entity;
	std::shared_ptr<Net::Download> m_dl;
};
}
