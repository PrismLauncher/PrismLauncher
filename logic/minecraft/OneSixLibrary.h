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

#include <QString>
#include <QStringList>
#include <QMap>
#include <QJsonObject>
#include <QDir>
#include <memory>

#include "net/URLConstants.h"
#include "minecraft/OpSys.h"
#include "minecraft/RawLibrary.h"

class OneSixLibrary;
typedef std::shared_ptr<OneSixLibrary> OneSixLibraryPtr;

/**
 * This is a leftover from a previous design.
 * All it does is separate the 'Raw' libraries read from files from the 'OneSix' libraries
 * used for actually doing things.
 *
 * DEPRECATED, but still useful to keep the data clean and separated by type.
 */
class OneSixLibrary : public RawLibrary
{
public:
	/// Constructor
	OneSixLibrary(const QString &name)
	{
		m_name = name;
	}
	/// Constructor
	OneSixLibrary(RawLibraryPtr base);
	static OneSixLibraryPtr fromRawLibrary(RawLibraryPtr lib);
};
