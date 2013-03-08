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

#include "stdinstancetype.h"

#include <QDir>
#include <QFileInfo>

#include "stdinstance.h"
#include "stdinstversionlist.h"

StdInstanceType::StdInstanceType(QObject *parent) :
	QObject(parent)
{
	
}

InstVersionList *StdInstanceType::versionList() const
{
	return &vList;
}

InstanceLoader::InstTypeError StdInstanceType::createInstance(Instance *&inst, 
															  const QString &instDir) const
{
	QFileInfo rootDir(instDir);
	
	if (!rootDir.exists() && !QDir().mkdir(rootDir.path()))
	{
		return InstanceLoader::CantCreateDir;
	}
	
	StdInstance *stdInst = new StdInstance(instDir, this);
	
	// TODO: Verify that the instance is valid.
	
	inst = stdInst;
	
	return InstanceLoader::NoError;
}

InstanceLoader::InstTypeError StdInstanceType::loadInstance(Instance *&inst, 
															const QString &instDir) const
{
	StdInstance *stdInst = new StdInstance(instDir, this);
	
	// TODO: Verify that the instance is valid.
	
	inst = stdInst;
	
	return InstanceLoader::NoError;
}
