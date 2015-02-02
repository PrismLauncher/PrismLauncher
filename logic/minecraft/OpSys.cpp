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

#include "OpSys.h"

OpSys OpSys_fromString(QString name)
{
	if (name == "linux")
		return Os_Linux;
	if (name == "windows")
		return Os_Windows;
	if (name == "osx")
		return Os_OSX;
	return Os_Other;
}

QString OpSys_toString(OpSys name)
{
	switch (name)
	{
	case Os_Linux:
		return "linux";
	case Os_OSX:
		return "osx";
	case Os_Windows:
		return "windows";
	default:
		return "other";
	}
}