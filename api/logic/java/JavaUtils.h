/* Copyright 2013-2017 MultiMC Contributors
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

#include <QStringList>

#include "JavaChecker.h"
#include "JavaInstallList.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "multimc_logic_export.h"

QProcessEnvironment CleanEnviroment();

class MULTIMC_LOGIC_EXPORT JavaUtils : public QObject
{
	Q_OBJECT
public:
	JavaUtils();

	JavaInstallPtr MakeJavaPtr(QString path, QString id = "unknown", QString arch = "unknown");
	QList<QString> FindJavaPaths();
	JavaInstallPtr GetDefaultJava();

#ifdef Q_OS_WIN
	QList<JavaInstallPtr> FindJavaFromRegistryKey(DWORD keyType, QString keyName);
#endif
};
