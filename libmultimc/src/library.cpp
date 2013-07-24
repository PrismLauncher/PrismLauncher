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

#include "include/library.h"

RuleAction RuleAction_fromString(QString name)
{
	if(name == "allow")
		return Allow;
	if(name == "disallow")
		return Disallow;
	return Defer;
}

OpSys OpSys_fromString(QString name)
{
	if(name == "linux")
		return Os_Linux;
	if(name == "windows")
		return Os_Windows;
	if(name == "osx")
		return Os_OSX;
	return Os_Other;
}
// default url for lib: https://s3.amazonaws.com/Minecraft.Download/libraries/
