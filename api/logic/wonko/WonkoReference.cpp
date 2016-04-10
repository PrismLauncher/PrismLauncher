/* Copyright 2015 MultiMC Contributors
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

#include "WonkoReference.h"

WonkoReference::WonkoReference(const QString &uid)
	: m_uid(uid)
{
}

QString WonkoReference::uid() const
{
	return m_uid;
}

QString WonkoReference::version() const
{
	return m_version;
}
void WonkoReference::setVersion(const QString &version)
{
	m_version = version;
}

bool WonkoReference::operator==(const WonkoReference &other) const
{
	return m_uid == other.m_uid && m_version == other.m_version;
}
bool WonkoReference::operator!=(const WonkoReference &other) const
{
	return m_uid != other.m_uid || m_version != other.m_version;
}
