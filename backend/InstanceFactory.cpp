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

#include "InstanceFactory.h"

#include <QDir>
#include <QFileInfo>

#include "BaseInstance.h"

#include "inifile.h"

#include "pathutils.h"

InstanceFactory InstanceFactory::loader;

InstanceFactory::InstanceFactory() :
	QObject(NULL)
{
	
}

InstanceFactory::InstLoadError InstanceFactory::loadInstance(BaseInstance *&inst, const QString &instDir)
{
	BaseInstance *loadedInst = new BaseInstance(instDir, this);
	
	// TODO: Sanity checks to verify that the instance is valid.
	
	inst = loadedInst;
	
	return NoLoadError;
}


InstanceFactory::InstCreateError InstanceFactory::createInstance(BaseInstance *&inst, const QString &instDir)
{
	QDir rootDir(instDir);
	
	qDebug(instDir.toUtf8());
	if (!rootDir.exists() && !rootDir.mkpath("."))
	{
		return InstanceFactory::CantCreateDir;
	}
	
	inst = new BaseInstance(instDir, this);
	
	//FIXME: really, how do you even know?
	return InstanceFactory::NoCreateError;
}
