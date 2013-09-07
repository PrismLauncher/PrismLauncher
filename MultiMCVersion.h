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

#include <QString>

/*!
 * \brief The Version class represents a MultiMC version number.
 */
struct MultiMCVersion
{
	/*!
	 * \brief Converts the Version to a string.
	 * \return The version number in string format (major.minor.revision.build).
	 */
	QString toString() const
	{
		return QString("%1.%2.%3.%4").arg(
					QString::number(major),
					QString::number(minor),
					QString::number(revision),
					QString::number(build));
	}

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
};

