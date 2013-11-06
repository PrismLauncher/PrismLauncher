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

#include <QStringList>
#include <QWidget>

#include <osutils.h>

#include "logic/lists/JavaVersionList.h"

#if WINDOWS
#include <windows.h>
#endif

class JavaUtils
{
public:
	JavaUtils();

	QList<JavaVersionPtr> FindJavaPaths();
	JavaVersionPtr GetDefaultJava();

private:

#if WINDOWS
	QList<JavaVersionPtr> FindJavaFromRegistryKey(DWORD keyType, QString keyName);
#endif
};
