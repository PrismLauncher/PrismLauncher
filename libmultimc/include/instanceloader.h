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

#ifndef INSTANCELOADER_H
#define INSTANCELOADER_H

#include <QObject>
#include <QMap>
#include <QList>

#include "libmmc_config.h"

class InstanceTypeInterface;
class Instance;

typedef QList<const InstanceTypeInterface *> InstTypeList;

/*!
 * \brief The InstanceLoader is a singleton that manages all of the instance types and handles loading and creating instances.
 * Instance types are registered with the instance loader through its registerInstType() function. 
 * Creating instances is done through the InstanceLoader's createInstance() function. This function takes 
 */
class LIBMULTIMC_EXPORT InstanceLoader : public QObject
{
	Q_OBJECT
public:
	/*!
	 * \brief Gets a reference to the instance loader.
	 */
	static InstanceLoader &get() { return loader; }
	
	/*!
	 * \brief Error codes returned by functions in the InstanceLoader and InstanceType classes.
	 *
	 * - NoError indicates that no error occurred.
	 * - OtherError indicates that an unspecified error occurred.
	 * - TypeIDExists is returned	by registerInstanceType() if the ID of the type being registered already exists.
	 * - TypeNotRegistered is returned by createInstance() and loadInstance() when the given type is not registered.
	 * - InstExists is returned by createInstance() if the given instance directory is already an instance.
	 * - NotAnInstance is returned by loadInstance() if the given instance directory is not a valid instance.
	 * - WrongInstType is returned by loadInstance() if the given instance directory's type doesn't match the given type.
	 * - CantCreateDir is returned by createInstance( if the given instance directory can't be created.)
	 */
	enum InstTypeError
	{
		NoError = 0,
		OtherError,
		
		TypeIDExists,
		
		TypeNotRegistered,
		InstExists,
		NotAnInstance,
		WrongInstType,
		CantCreateDir
	};
	
	/*!
	 * \brief Registers the given InstanceType with the instance loader.
	 *
	 * \param type The InstanceType to register.
	 * \return An InstTypeError error code.
	 * - TypeIDExists if the given type's is already registered to another instance type.
	 */
	InstTypeError registerInstanceType(InstanceTypeInterface *type);
	
	/*!
	 * \brief Creates an instance with the given type and stores it in inst.
	 *
	 * \param inst Pointer to store the created instance in.
	 * \param type The type of instance to create.
	 * \param instDir The instance's directory.
	 * \return An InstTypeError error code.
	 * - TypeNotRegistered if the given type is not registered with the InstanceLoader.
	 * - InstExists if the given instance directory is already an instance.
	 * - CantCreateDir if the given instance directory cannot be created.
	 */
	InstTypeError createInstance(Instance *&inst, const InstanceTypeInterface *type, const QString &instDir);
	
	/*!
	 * \brief Loads an instance from the given directory.
	 *
	 * \param inst Pointer to store the loaded instance in.
	 * \param type The type of instance to load.
	 * \param instDir The instance's directory.
	 * \return An InstTypeError error code.
	 * - TypeNotRegistered if the given type is not registered with the InstanceLoader.
	 * - NotAnInstance if the given instance directory isn't a valid instance.
	 * - WrongInstType if the given instance directory's type isn't the same as the given type.
	 */
	InstTypeError loadInstance(Instance *&inst, const InstanceTypeInterface *type, const QString &instDir);
	
	/*!
	 * \brief Loads an instance from the given directory.
	 * Checks the instance's INI file to figure out what the instance's type is first.
	 * \param inst Pointer to store the loaded instance in.
	 * \param instDir The instance's directory.
	 * \return An InstTypeError error code.
	 * - TypeNotRegistered if the instance's type is not registered with the InstanceLoader.
	 * - NotAnInstance if the given instance directory isn't a valid instance.
	 */
	InstTypeError loadInstance(Instance *&inst, const QString &instDir);
	
	/*!
	 * \brief Finds an instance type with the given ID.
	 * If one cannot be found, returns NULL.
	 *
	 * \param id The ID of the type to find.
	 * \return The type with the given ID. NULL if none were found.
	 */
	const InstanceTypeInterface *findType(const QString &id);
	
	/*!
	 * \brief Gets a list of the registered instance types.
	 *
	 * \return A list of instance types.
	 */
	InstTypeList typeList();
	
private:
	InstanceLoader();
	
	QMap<QString, InstanceTypeInterface *> m_typeMap;
	
	static InstanceLoader loader;
};

#endif // INSTANCELOADER_H
