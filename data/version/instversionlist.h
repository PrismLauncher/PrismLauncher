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

#ifndef INSTVERSIONLIST_H
#define INSTVERSIONLIST_H

#include <QObject>

class InstVersion;

// Class that each instance type's version list derives from. Version lists are 
// the lists that keep track of the available game versions for that instance. 
// This list will not be loaded on startup. It will be loaded when the list's 
// load function is called.
class InstVersionList : public QObject
{
	Q_OBJECT
public:
	explicit InstVersionList();
	
	// Reloads the version list.
	virtual void loadVersionList() = 0;
	
	// Gets the version at the given index.
	virtual const InstVersion *at(int i) const = 0;
	
	// Returns the number of versions in the list.
	virtual int count() const = 0;
};

#endif // INSTVERSIONLIST_H
