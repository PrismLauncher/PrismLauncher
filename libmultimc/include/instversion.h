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

#ifndef INSTVERSION_H
#define INSTVERSION_H

#include <QObject>

#include "libmmc_config.h"

class InstVersionList;

class LIBMULTIMC_EXPORT InstVersion : public QObject
{
	Q_OBJECT
public:
	// Constructs a new InstVersion with the given parent. The parent *must*
	// be the InstVersionList that contains this InstVersion. The InstVersion
	// should be added to the list immediately after being created as any calls
	// to id() will likely fail unless the InstVersion is in a list.
	explicit InstVersion(InstVersionList *parent = 0);
	
	// Returns this InstVersion's ID. This is usually just the InstVersion's index
	// within its InstVersionList, but not always.
	// If this InstVersion is not in an InstVersionList, returns -1.
	virtual int id() const = 0;
	
	// Returns this InstVersion's name. This is displayed to the user in the GUI
	// and is usually just the version number ("1.4.7"), for example.
	virtual QString name() const = 0;
	
	// Returns this InstVersion's name. This is usually displayed to the user 
	// in the GUI and specifies what kind of version this is. For example: it 
	// could be "Snapshot", "Latest Version", "MCNostalgia", etc.
	virtual QString type() const = 0;
	
	// Returns the version list that this InstVersion is a part of.
	virtual InstVersionList *versionList() const;
};

#endif // INSTVERSION_H
