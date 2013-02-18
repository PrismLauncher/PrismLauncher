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

#include "instance.h"

#include <QFileInfo>

#include "util/pathutils.h"

Instance::Instance(const QString &rootDir, QObject *parent) :
	SettingsBase(parent)
{
	m_rootDir = rootDir;
	config.loadFile(PathCombine(rootDir, "instance.cfg"));
}

QString Instance::id()
{
	return QFileInfo(rootDir()).baseName();
}

QString Instance::rootDir()
{
	return m_rootDir;
}

InstanceList *Instance::instList()
{
	if (parent()->inherits("InstanceList"))
		return (InstanceList *)parent();
	else
		return NULL;
}

QVariant Instance::getField(const QString &name, QVariant defVal) const
{
	return config.get(name, defVal);
}

void Instance::setField(const QString &name, QVariant val)
{
	config.set(name, val);
}
