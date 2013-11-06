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

#include "include/basicsettingsobject.h"
#include "include/setting.h"

BasicSettingsObject::BasicSettingsObject(QObject *parent) : SettingsObject(parent)
{
}

void BasicSettingsObject::changeSetting(const Setting &setting, QVariant value)
{
	if (contains(setting.id()))
	{
		if (value.isValid())
			config.setValue(setting.configKey(), value);
		else
			config.remove(setting.configKey());
	}
}

QVariant BasicSettingsObject::retrieveValue(const Setting &setting)
{
	if (contains(setting.id()))
	{
		return config.value(setting.configKey());
	}
	else
	{
		return QVariant();
	}
}
