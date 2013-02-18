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

#include "instanceloader.h"

#include "instancetype.h"

InstanceLoader::InstanceLoader() :
	QObject(NULL)
{
	
}


InstanceLoader::InstTypeError InstanceLoader::registerInstanceType(InstanceType *type)
{
	// Check to see if the type ID exists.
	if (m_typeMap.contains(type->typeID()))
		return TypeIDExists;
	
	// Set the parent to this.
	type->setParent(this);
	
	// Add it to the map.
	m_typeMap.insert(type->typeID(), type);
	return NoError;
}

InstanceLoader::InstTypeError InstanceLoader::createInstance(Instance *inst, 
															 const InstanceType *type, 
															 const QString &instDir)
{
	// Check if the type is registered.
	if (!type || findType(type->typeID()) != type)
		return TypeNotRegistered;
	
	// Create the instance.
	return type->createInstance(inst, instDir);
}

InstanceLoader::InstTypeError InstanceLoader::loadInstance(Instance *inst, 
														   const InstanceType *type, 
														   const QString &instDir)
{
	// Check if the type is registered.
	if (!type || findType(type->typeID()) != type)
		return TypeNotRegistered;
	
	return type->loadInstance(inst, instDir);
}

const InstanceType *InstanceLoader::findType(const QString &id)
{
	if (!m_typeMap.contains(id))
		return NULL;
	else
		return m_typeMap[id];
}

InstTypeList InstanceLoader::typeList()
{
	InstTypeList typeList;
	
	for (auto iter = m_typeMap.begin(); iter != m_typeMap.end(); iter++)
	{
		typeList.append(*iter);
	}
	
	return typeList;
}
