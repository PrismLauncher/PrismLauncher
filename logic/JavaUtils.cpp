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

#include "JavaUtils.h"
#include "osutils.h"
#include "pathutils.h"

#include <QStringList>
#include <QString>
#include <QDir>
#include <logger/QsLog.h>

#if WINDOWS
#include <windows.h>

#endif

JavaUtils::JavaUtils()
{

}

#if WINDOWS
QStringList JavaUtils::FindJavaPath()
{
	QStringList paths;

	HKEY jreKey;
	QString jreKeyName = "SOFTWARE\\JavaSoft\\Java Runtime Environment";
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, jreKeyName.toStdString().c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &jreKey) == ERROR_SUCCESS)
	{
		// Read the current JRE version from the registry.
		// This will be used to find the key that contains the JavaHome value.
		char *value = new char[0];
		DWORD valueSz = 0;
		if (RegQueryValueExA(jreKey, "CurrentVersion", NULL, NULL, (BYTE*)value, &valueSz) == ERROR_MORE_DATA)
		{
			value = new char[valueSz];
			RegQueryValueExA(jreKey, "CurrentVersion", NULL, NULL, (BYTE*)value, &valueSz);
		}

		RegCloseKey(jreKey);

		// Now open the registry key for the JRE version that we just got.
		jreKeyName.append("\\").append(value);
		if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, jreKeyName.toStdString().c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &jreKey) == ERROR_SUCCESS)
		{
			// Read the JavaHome value to find where Java is installed.
			value = new char[0];
			valueSz = 0;
			if (RegQueryValueExA(jreKey, "JavaHome", NULL, NULL, (BYTE*)value, &valueSz) == ERROR_MORE_DATA)
			{
				value = new char[valueSz];
				RegQueryValueExA(jreKey, "JavaHome", NULL, NULL, (BYTE*)value, &valueSz);

				paths << QDir(PathCombine(value, "bin")).absoluteFilePath("java.exe");
			}

			RegCloseKey(jreKey);
		}
	}

	if(paths.length() <= 0)
	{
		QLOG_WARN() << "Failed to find Java in the Windows registry - defaulting to \"java\"";
		paths << "java";
	}

	return paths;
}
#elif OSX
QStringList JavaUtils::FindJavaPath()
{
	QLOG_INFO() << "OS X Java detection incomplete - defaulting to \"java\"";

	QStringList paths;
	paths << "java";

	return paths;
}

#elif LINUX
QStringList JavaUtils::FindJavaPath()
{
	QLOG_INFO() << "Linux Java detection incomplete - defaulting to \"java\"";

	QStringList paths;
	paths << "java";

	return paths;
}
#else
QStringList JavaUtils::FindJavaPath()
{
	QLOG_INFO() << "Unknown operating system build - defaulting to \"java\"";

	QStringList paths;
	paths << "java";

	return paths;
}
#endif
