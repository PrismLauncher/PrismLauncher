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

#include "stdinstance.h"

#include <QFileInfo>

#include <setting.h>

#include <javautils.h>

#include "stdinstversionlist.h"

StdInstance::StdInstance(const QString &rootDir, const InstanceTypeInterface *iType, QObject *parent) :
	Instance(rootDir, parent)
{
	m_instType = iType;
	
	settings().registerSetting(new Setting("lastVersionUpdate", 0));
}

bool StdInstance::shouldUpdateCurrentVersion()
{
	QFileInfo jar(mcJar());
	return jar.lastModified().toUTC().toMSecsSinceEpoch() != lastVersionUpdate();
}

void StdInstance::updateCurrentVersion(bool keepCurrent)
{
	QFileInfo jar(mcJar());
	
	if(!jar.exists())
	{
		setLastVersionUpdate(0);
		setCurrentVersion("Unknown");
		return;
	}
	
	qint64 time = jar.lastModified().toUTC().toMSecsSinceEpoch();
	
	setLastVersionUpdate(time);
	if (!keepCurrent)
	{
		// TODO: Implement GetMinecraftJarVersion function.
		QString newVersion = "Unknown";//javautils::GetMinecraftJarVersion(jar.absoluteFilePath());
		setCurrentVersion(newVersion);
	}
}

const InstanceTypeInterface *StdInstance::instanceType() const
{
	return m_instType;
}

InstVersionList *StdInstance::versionList() const
{
	return &vList;
}
