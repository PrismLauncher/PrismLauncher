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

#include "include/instversion.h"
#include "include/instversionlist.h"

InstVersion::InstVersion(const QString &descriptor, 
						 const QString &name, 
						 qint64 timestamp, 
						 InstVersionList *parent) :
	QObject(parent), m_descriptor(descriptor), m_name(name), m_timestamp(timestamp)
{
	
}

InstVersion::InstVersion(const InstVersion &other, QObject *parent) :
	QObject(parent ? parent : other.parent()), 
	m_descriptor(other.descriptor()), m_name(other.name()), m_timestamp(other.timestamp())
{
	
}

InstVersionList *InstVersion::versionList() const
{
	// Parent should *always* be either an InstVersionList or NULL.
	if (!parent() || !parent()->inherits("InstVersionList"))
		return NULL;
	else
		return (InstVersionList *)parent();
}

bool InstVersion::isLessThan(const InstVersion &other) const
{
	return timestamp() < other.timestamp();
}

bool InstVersion::isGreaterThan(const InstVersion &other) const
{
	return timestamp() > other.timestamp();
}

QString InstVersion::descriptor() const
{
	return m_descriptor;
}

QString InstVersion::name() const
{
	return m_name;
}

qint64 InstVersion::timestamp() const
{
	return m_timestamp;
}
