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

class InstanceType;
class Instance;

/*!
 * \brief The InstanceLoader is a singleton that manages all of the instance types and handles loading and creating instances.
 * Instance types are registered with the instance loader through its registerInstType() function. 
 * Creating instances is done through the InstanceLoader's createInstance() function. This function takes 
 */
class InstanceLoader : public QObject
{
	Q_OBJECT
public:
	static InstanceLoader instLoader;
	
	/*!
	 * \brief Error codes returned by functions in the InstanceLoader and InstanceType classes.
	 * NoError indicates that no error occurred.
	 * OtherError indicates that an unspecified error occurred.
	 * TypeIDExists is returned	by registerInstanceType() if the ID of the type being registered already exists.
	 * 
	 * TypeNotRegistered is returned by createInstance() and loadInstance() when the given type is not registered.
	 * InstExists is returned by createInstance() if the given instance directory is already an instance.
	 * NotAnInstance is returned by loadInstance() if the given instance directory is not a valid instance.
	 */
	enum InstTypeError
	{
		NoError = 0,
		OtherError,
		
		TypeIDExists,
		
		TypeNotRegistered,
		InstExists,
		NotAnInstance
	};
	
	/*!
	 * \brief Registers the given InstanceType with the instance loader.
	 *        This causes the instance loader to take ownership of the given 
	 *        instance type (meaning the instance type's parent will be set to 
	 *        the instance loader).
	 * \param type The InstanceType to register.
	 * \return An InstTypeError error code.
	 *         TypeIDExists if the given type's is already registered to another instance type.
	 */
	InstTypeError registerInstanceType(InstanceType *type);
	
	/*!
	 * \brief Creates an instance with the given type and stores it in inst.
	 * \param inst Pointer to store the created instance in.
	 * \param type The type of instance to create.
	 * \param instDir The instance's directory.
	 * \return An InstTypeError error code.
	 *         TypeNotRegistered if the given type is not registered with the InstanceLoader.
	 *         InstExists if the given instance directory is already an instance.
	 */
	InstTypeError createInstance(Instance *inst, const InstanceType *type, const QString &instDir);
	
	/*!
	 * \brief Loads an instance from the given directory.
	 * \param inst Pointer to store the loaded instance in.
	 * \param type The type of instance to load.
	 * \param instDir The instance's directory.
	 * \return An InstTypeError error code.
	 *         TypeNotRegistered if the given type is not registered with the InstanceLoader.
	 *         NotAnInstance if the given instance directory isn't a valid instance.
	 */
	InstTypeError loadInstance(Instance *inst, const InstanceType *type, const QString &instDir);
	
private:
	explicit InstanceLoader(QObject *parent = 0);
};

#endif // INSTANCELOADER_H
