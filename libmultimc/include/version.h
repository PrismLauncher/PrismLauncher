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

#ifndef VERSION_H
#define VERSION_H

#include <QObject>

#include "libmmc_config.h"

/*!
 * \brief The Version class represents a MultiMC version number.
 */
class LIBMULTIMC_EXPORT Version : public QObject
{
	Q_OBJECT
public:
	explicit Version(int major = 0, int minor = 0, int revision = 0, 
					 int build = 0, QObject *parent = 0);
	
	Version(const Version& ver);
	
	/*!
	 * \brief Converts the Version to a string.
	 * \return The version number in string format (major.minor.revision.build).
	 */
	QString toString() const;
	
	/*!
	 * \brief The major version number.
	 * For MultiMC 5, this will always be 5.
	 */
	int major;
	
	/*!
	 * \brief The minor version number.
	 * This number is incremented when major features are added.
	 */
	int minor;
	
	/*!
	 * \brief The revision number.
	 * This number is incremented for bugfixes and small features.
	 */
	int revision;
	
	/*!
	 * \brief The build number.
	 * This number is automatically set by Jenkins. It is incremented every time
	 * a new build is run.
	 */
	int build;
	
	static Version current;
};

#endif // VERSION_H
