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
		QString vstr = QString("%1.%2").arg(
				QString::number(major),
				QString::number(minor));

		if (build > 0) vstr += QString(".%1").arg(QString::number(build));
		if (!buildType.isEmpty()) vstr += QString("-%1").arg(buildType);

		return vstr;
	}

	/*!
	 * \brief The major version number.
	 * This is no longer going to always be 5 for MultiMC 5. Doing so is useless.
	 * Instead, we'll be starting major off at 1 and incrementing it with every major feature.
	 */
	int major;
	
	/*!
	 * \brief The minor version number.
	 * This number is incremented for major features and bug fixes.
	 */
	int minor;
	
	/*!
	 * \brief The build number.
	 * This number is automatically set by Buildbot it is set to the build number of the buildbot 
	 * build that this build came from.
	 * If this build didn't come from buildbot and no build number was given to CMake, this will default
	 * to -1, causing it to not show in this version's string representation.
	 */
	int build;

	/*!
	 * \brief The build type.
	 * This indicates the type of build that this is. For example, lin64-stable.
	 * Usually corresponds to this build's buildbot builder name.
	 */
	QString buildType;
};

