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

#pragma once

#include <QObject>
#include <memory>

#include "multimc_logic_export.h"

class Task;

class MULTIMC_LOGIC_EXPORT BaseWonkoEntity
{
public:
	virtual ~BaseWonkoEntity();

	using Ptr = std::shared_ptr<BaseWonkoEntity>;

	virtual std::unique_ptr<Task> remoteUpdateTask() = 0;
	virtual std::unique_ptr<Task> localUpdateTask() = 0;
	virtual void merge(const std::shared_ptr<BaseWonkoEntity> &other) = 0;

	void store() const;
	virtual QString localFilename() const = 0;
	virtual QJsonObject serialized() const = 0;

	bool isComplete() const { return m_localLoaded || m_remoteLoaded; }

	bool isLocalLoaded() const { return m_localLoaded; }
	bool isRemoteLoaded() const { return m_remoteLoaded; }

	void notifyLocalLoadComplete();
	void notifyRemoteLoadComplete();

private:
	bool m_localLoaded = false;
	bool m_remoteLoaded = false;
};
