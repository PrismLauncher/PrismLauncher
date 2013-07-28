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

#include "AppVersion.h"

#include "config.h"

Version Version::current(VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION, VERSION_BUILD);

Version::Version(int major, int minor, int revision, int build, QObject *parent) :
	QObject(parent)
{
	this->major = major;
	this->minor = minor;
	this->revision = revision;
	this->build = build;
}

Version::Version(const Version& ver)
{
	this->major = ver.major;
	this->minor = ver.minor;
	this->revision = ver.revision;
	this->build = ver.build;
}

QString Version::toString() const
{
	return QString("%1.%2.%3.%4").arg(
				QString::number(major),
				QString::number(minor),
				QString::number(revision),
				QString::number(build));
}
