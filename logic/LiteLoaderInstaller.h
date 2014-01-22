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
#include <QString>
#include <QMap>
#include <memory>

class DerpVersion;
class DerpInstance;
class QDir;

// TODO base class
class LiteLoaderInstaller
{
public:
	LiteLoaderInstaller();

	bool canApply(DerpInstance *instance) const;
	bool isApplied(DerpInstance *on);

	bool add(DerpInstance *to);
	bool remove(DerpInstance *from);

private:
	virtual QString id() const { return "com.mumfrey.liteloader"; }
	QString filename(const QString &root) const;
	QDir patchesDir(const QString &root) const;

	static QMap<QString, QString> m_launcherWrapperVersionMapping;
};
