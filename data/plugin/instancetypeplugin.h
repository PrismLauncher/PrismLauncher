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

#ifndef INSTANCETYPEPLUGIN_H
#define INSTANCETYPEPLUGIN_H

#include <QList>

#include "data/inst/instancetype.h"

/*!
 * \brief Interface for plugins that want to provide custom instance types.
 */
class InstanceTypePlugin
{
public:
	/*!
	 * \brief Gets a QList containing the instance types that this plugin provides.
	 * These instance types are then registered with the InstanceLoader.
	 * The InstanceType objects should \e not be deleted by the plugin. Once they 
	 * are registered, they belong to the InstanceLoader.
	 * \return A QList containing this plugin's instance types.
	 */
	virtual QList<InstanceType *> getInstanceTypes() = 0;
};

Q_DECLARE_INTERFACE(InstanceTypePlugin, "net.forkk.MultiMC.InstanceTypePlugin/0.1")

#endif // INSTANCETYPEPLUGIN_H
