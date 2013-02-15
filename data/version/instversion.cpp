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

#include "instversion.h"
#include "instversionlist.h"

InstVersion::InstVersion(InstVersionList *parent) :
	QObject(parent)
{
	
}

InstVersionList *InstVersion::versionList() const
{
	// Parent should *always* be an InstVersionList
	if (!parent() || !parent()->inherits("InstVersionList"))
		return NULL;
	else
		return (InstVersionList *)parent();
}
