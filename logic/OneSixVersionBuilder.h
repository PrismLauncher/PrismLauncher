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

class OneSixVersion;
class OneSixInstance;
class QWidget;
class QJsonObject;
class QFileInfo;

class OneSixVersionBuilder
{
	OneSixVersionBuilder();
public:
	static bool build(OneSixVersion *version, OneSixInstance *instance, QWidget *widgetParent);
	static bool read(OneSixVersion *version, const QJsonObject &obj);

private:
	OneSixVersion *m_version;
	OneSixInstance *m_instance;
	QWidget *m_widgetParent;

	enum Type
	{
		Override,
		Add,
		Remove
	};

	bool build();
	bool read(const QJsonObject &obj);

	void clear();
	bool apply(const QJsonObject &object);
	bool applyLibrary(const QJsonObject &lib, const Type type);

	bool read(const QFileInfo &fileInfo, QJsonObject *out);
};
