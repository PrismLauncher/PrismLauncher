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
 * \brief The Version class represents a MultiMC version.
 */
struct MultiMCVersion
{
	enum Type
	{
		//! Version type for stable release builds.
		Release,

		//! Version type for release candidates.
		ReleaseCandidate,

		//! Version type for development builds.
		Development,

		//! Version type for custom builds. This is the default when no version type is specified.
		Custom
	};

	/*!
	 * \brief Converts the Version to a string.
	 * \return The version number in string format (major.minor.revision.build).
	 */
	QString toString() const
	{
		QString vstr = QString("%1.%2").arg(
				QString::number(major),
				QString::number(minor));

		if (hotfix > 0) vstr += "." + QString::number(hotfix);

		// If the build is a development build or release candidate, add that info to the end.
		if (type == Development) vstr += "-dev" + QString::number(build);
		else if (type == ReleaseCandidate) vstr += "-rc" + QString::number(build);

		return vstr;
	}

	QString typeName() const
	{
		switch (type)
		{
			case Release:
				return "Stable Release";
			case ReleaseCandidate:
				return "Release Candidate";
			case Development:
				return "Development";
			case Custom:
			default:
				return "Custom";
		}
	}

	//! The major version number.
	int major;
	
	//! The minor version number.
	int minor;
	
	//! The hotfix number.
	int hotfix;

	//! The build number.
	int build;

	//! The build type.
	Type type;

	//! The build channel.
	QString channel;

	//! A short string identifying the platform that this version is for. For example, lin64 or win32.
	QString platform;
};

