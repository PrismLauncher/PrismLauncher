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

class DerpVersion;
class DerpInstance;
class QWidget;
class QJsonObject;
class QFileInfo;

class DerpVersionBuilder
{
	DerpVersionBuilder();
public:
	static bool build(DerpVersion *version, DerpInstance *instance, QWidget *widgetParent);

private:
	DerpVersion *m_version;
	DerpInstance *m_instance;
	QWidget *m_widgetParent;

	enum Type
	{
		Override,
		Add,
		Remove
	};

	bool build();

	void clear();
	bool apply(const QJsonObject &object);
	bool applyLibrary(const QJsonObject &lib, const Type type);

	bool read(const QFileInfo &fileInfo, QJsonObject *out);
};
