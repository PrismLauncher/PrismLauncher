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

#include "instancebase.h"

#include "../util/pathutils.h"

InstanceBase::InstanceBase(QString rootDir, QObject *parent) :
	QObject(parent), 
	m_rootDir(rootDir),
	m_config(PathCombine(rootDir, "instance.cfg"), QSettings::IniFormat)
{
	
}

QString InstanceBase::GetRootDir() const
{
	return m_rootDir;
}


///////////// Config Values /////////////

// Name
QString InstanceBase::GetInstName() const
{
	return m_config.value("name", "Unnamed").toString();
}

void InstanceBase::SetInstName(QString name)
{
	m_config.setValue("name", name);
}
