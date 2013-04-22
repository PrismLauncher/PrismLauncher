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

#include "include/inisettingsobject.h"
#include "include/setting.h"

INISettingsObject::INISettingsObject(const QString &path, QObject *parent) :
	SettingsObject(parent)
{
	m_filePath = path;
	m_ini.loadFile(path);
}

void INISettingsObject::setFilePath(const QString &filePath)
{
	m_filePath = filePath;
}

void INISettingsObject::changeSetting(const Setting &setting, QVariant value)
{
	if (contains(setting.id()))
	{
		if(value.isValid())
			m_ini.set(setting.configKey(), value);
		else
			m_ini.remove(setting.configKey());
		m_ini.saveFile(m_filePath);
	}
}

QVariant INISettingsObject::retrieveValue(const Setting &setting)
{
	if (contains(setting.id()))
	{
		return m_ini.get(setting.configKey(), QVariant());
	}
	else
	{
		return QVariant();
	}
}
