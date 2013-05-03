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

class Instance;

/*!
 * The InstanceLoader is a singleton that manages loading and creating instances.
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
	 * - InstExists is returned by createInstance() if the given instance directory is already an instance.
	 * - NotAnInstance is returned by loadInstance() if the given instance directory is not a valid instance.
	 * - CantCreateDir is returned by createInstance( if the given instance directory can't be created.)
	 */
	enum InstLoaderError
	{
		NoError = 0,
		OtherError,
		
		InstExists,
		NotAnInstance,
		CantCreateDir
	};
	
	/*!
	 * \brief Creates an instance with the given type and stores it in inst.
	 *
	 * \param inst Pointer to store the created instance in.
	 * \param type The type of instance to create.
	 * \param instDir The instance's directory.
	 * \return An InstLoaderError error code.
	 * - InstExists if the given instance directory is already an instance.
	 * - CantCreateDir if the given instance directory cannot be created.
	 */
	InstLoaderError createInstance(Instance *&inst, const QString &instDir);
	
	/*!
	 * \brief Loads an instance from the given directory.
	 * Checks the instance's INI file to figure out what the instance's type is first.
	 * \param inst Pointer to store the loaded instance in.
	 * \param instDir The instance's directory.
	 * \return An InstLoaderError error code.
	 * - NotAnInstance if the given instance directory isn't a valid instance.
	 */
	InstLoaderError loadInstance(Instance *&inst, const QString &instDir);
	
private:
	InstanceLoader();
	
	static InstanceLoader loader;
};

#endif // INSTANCELOADER_H
