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

#include "osutils.h"

#if WINDOWS
	#include <windows.h>
#endif

#define JI_ID 0
#define JI_ARCH 1
#define JI_PATH 2
#define JI_REC 3
typedef std::tuple<QString, QString, QString, bool> java_install;

class JavaUtils
{
public:
	JavaUtils();

	std::vector<java_install> FindJavaPaths();

private:
	std::vector<java_install> GetDefaultJava();
#if WINDOWS
	std::vector<java_install> FindJavaFromRegistryKey(DWORD keyType, QString keyName);
#endif
};
