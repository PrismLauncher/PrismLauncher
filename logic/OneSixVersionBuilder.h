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
#include "VersionFile.h"

class VersionFinal;
class OneSixInstance;
class QJsonObject;
class QFileInfo;

class OneSixVersionBuilder
{
	OneSixVersionBuilder();
public:
	static void build(VersionFinal *version, OneSixInstance *instance, const QStringList &external);
	static void readJsonAndApplyToVersion(VersionFinal *version, const QJsonObject &obj);

	static QMap<QString, int> readOverrideOrders(OneSixInstance *instance);
	static bool writeOverrideOrders(const QMap<QString, int> &order, OneSixInstance *instance);

private:
	VersionFinal *m_version;
	OneSixInstance *m_instance;

	void buildInternal(const QStringList& external);
	void readJsonAndApply(const QJsonObject &obj);

	VersionFilePtr parseJsonFile(const QFileInfo &fileInfo, const bool requireOrder,
							  bool isFTB = false);
};
