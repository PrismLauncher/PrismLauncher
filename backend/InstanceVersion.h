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

#pragma once
#include "libmmc_config.h"
#include <QSharedPointer>

/*!
 * An abstract base class for versions.
 */
struct LIBMULTIMC_EXPORT InstVersion
{
	/*!
	 * Checks if this version is less (older) than the given version.
	 * \param other The version to compare this one to.
	 * \return True if this version is older than the given version.
	 */
	virtual bool operator<(const InstVersion &rhs) const
	{
		return timestamp < rhs.timestamp;
	}

	/*!
	 * Checks if this version is greater (newer) than the given version.
	 * \param other The version to compare this one to.
	 * \return True if this version is newer than the given version.
	 */
	virtual bool operator>( const InstVersion& rhs ) const
	{
		return timestamp > rhs.timestamp;
	}
	
	/*!
	 * A string used to identify this version in config files.
	 * This should be unique within the version list or shenanigans will occur.
	 */
	QString descriptor;
	/*!
	 * The name of this version as it is displayed to the user.
	 * For example: "1.5.1"
	 */
	QString name;
	/*!
	 * Gets the version's timestamp.
	 * This is primarily used for sorting versions in a list.
	 */
	qint64 timestamp;
	
	virtual QString typeString() const
	{
		return "InstVersion";
	}
};

typedef QSharedPointer<InstVersion> InstVersionPtr;

Q_DECLARE_METATYPE( InstVersionPtr )