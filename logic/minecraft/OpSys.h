/* Copyright 2013-2015 MultiMC Contributors
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
enum OpSys
{
	Os_Windows,
	Os_Linux,
	Os_OSX,
	Os_Other
};

OpSys OpSys_fromString(QString);
QString OpSys_toString(OpSys);

#ifdef Q_OS_WIN32
#define currentSystem Os_Windows
#else
#ifdef Q_OS_MAC
#define currentSystem Os_OSX
#else
#define currentSystem Os_Linux
#endif
#endif