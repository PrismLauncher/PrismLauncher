/* Copyright 2013-2015 MultiMC Contributors
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

#pragma once

#include <QObject>
#include <QMap>
#include <QList>

#include "BaseVersion.h"
#include "BaseInstance.h"

struct BaseVersion;
class BaseInstance;

/*!
 * The \b InstanceFactory\b is a singleton that manages loading and creating instances.
 */
class InstanceFactory : public QObject
{
	Q_OBJECT
public:
	/*!
	 * \brief Gets a reference to the instance loader.
	 */
	static InstanceFactory &get()
	{
		return loader;
	}

	enum InstLoadError
	{
		NoLoadError = 0,
		UnknownLoadError,
		NotAnInstance
	};

	enum InstCreateError
	{
		NoCreateError = 0,
		NoSuchVersion,
		UnknownCreateError,
		InstExists,
		CantCreateDir
	};

	/*!
	 * \brief Creates a stub instance
	 *
	 * \param inst Pointer to store the created instance in.
	 * \param version Game version to use for the instance
	 * \param instDir The new instance's directory.
	 * \param type The type of instance to create
	 * \return An InstCreateError error code.
	 * - InstExists if the given instance directory is already an instance.
	 * - CantCreateDir if the given instance directory cannot be created.
	 */
	InstCreateError createInstance(InstancePtr &inst, BaseVersionPtr version,
								   const QString &instDir);

	/*!
	 * \brief Creates a copy of an existing instance with a new name
	 *
	 * \param newInstance Pointer to store the created instance in.
	 * \param oldInstance The instance to copy
	 * \param instDir The new instance's directory.
	 * \return An InstCreateError error code.
	 * - InstExists if the given instance directory is already an instance.
	 * - CantCreateDir if the given instance directory cannot be created.
	 */
	InstCreateError copyInstance(InstancePtr &newInstance, InstancePtr &oldInstance,
								 const QString &instDir);

	/*!
	 * \brief Loads an instance from the given directory.
	 * Checks the instance's INI file to figure out what the instance's type is first.
	 * \param inst Pointer to store the loaded instance in.
	 * \param instDir The instance's directory.
	 * \return An InstLoadError error code.
	 * - NotAnInstance if the given instance directory isn't a valid instance.
	 */
	InstLoadError loadInstance(InstancePtr &inst, const QString &instDir);

private:
	InstanceFactory();

	static InstanceFactory loader;
};
