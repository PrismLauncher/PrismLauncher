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

#ifndef STDINSTANCETYPE_H
#define STDINSTANCETYPE_H

#include <instancetypeinterface.h>

#define StdInstanceType_IID "net.forkk.MultiMC.StdInstanceType/0.1"

class StdInstanceType : public QObject, InstanceTypeInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StdInstanceType_IID FILE "stdinstance.json")
	Q_INTERFACES(InstanceTypeInterface)
public:
	explicit StdInstanceType(QObject *parent = 0);
	
	virtual QString typeID() const { return "net.forkk.MultiMC.StdInstance"; }
	
	virtual QString displayName() const { return "Standard Instance"; }
	
	virtual QString description() const { return "A standard Minecraft instance."; }
	
	virtual InstVersionList *versionList() const;
	
protected:
	virtual InstanceLoader::InstTypeError createInstance(Instance *&inst, const QString &instDir) const;
	
	virtual InstanceLoader::InstTypeError loadInstance(Instance *&inst, const QString &instDir) const;
};

#endif // STDINSTANCETYPE_H
