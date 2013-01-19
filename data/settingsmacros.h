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

#ifndef SETTINGSMACROS_H
#define SETTINGSMACROS_H

#define STR_VAL(val) # val

#define DEFINE_SETTING(funcName, name, defVal, typeName, toFunc) \
	virtual typeName Get ## funcName() const { return value(name). ## toFunc(); } \
	virtual void Set ## funcName(typeName value) { setValue(name, value); } \
	virtual void Reset ## funcName() { 

#define DEFINE_SETTING_STR(name, defVal) \
	DEFINE_SETTING(name, STR_VAL(name), defVal, QString, toString)

#define DEFINE_SETTING_BOOL(name, defVal) \
	DEFINE_SETTING(name, STR_VAL(name), defVal, bool, toBool)

#define DEFINE_SETTING_INT(name, defVal) \
	DEFINE_SETTING(name, STR_VAL(name), defVal, int, toInt)

#endif // SETTINGSMACROS_H
