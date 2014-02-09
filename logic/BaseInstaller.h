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

#pragma once

#include <memory>

class OneSixInstance;
class QDir;
class QString;

class BaseInstaller
{
public:
	BaseInstaller();

	virtual bool canApply(OneSixInstance *instance) const { return true; }
	bool isApplied(OneSixInstance *on);

	virtual bool add(OneSixInstance *to);
	virtual bool remove(OneSixInstance *from);

protected:
	virtual QString id() const = 0;
	QString filename(const QString &root) const;
	QDir patchesDir(const QString &root) const;
};
