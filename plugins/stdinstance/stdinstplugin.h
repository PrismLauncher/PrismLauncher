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

#ifndef STDINSTPLUGIN_H
#define STDINSTPLUGIN_H

#include <QObject>

#include <data/plugin/instancetypeplugin.h>

class StdInstPlugin : public QObject, InstanceTypePlugin
{
	Q_OBJECT
	Q_INTERFACES(InstanceTypePlugin)
	Q_PLUGIN_METADATA(IID "net.forkk.MultiMC.Plugins.StdInstance")
	
public:
	virtual QList<InstanceType *> getInstanceTypes();
};

#endif // STDINSTPLUGIN_H
