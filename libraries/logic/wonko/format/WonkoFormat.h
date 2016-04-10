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

#pragma once

#include <QJsonObject>
#include <memory>

#include "Exception.h"
#include "wonko/BaseWonkoEntity.h"

class WonkoIndex;
class WonkoVersion;
class WonkoVersionList;

class WonkoParseException : public Exception
{
public:
	using Exception::Exception;
};

class WonkoFormat
{
public:
	virtual ~WonkoFormat() {}

	static void parseIndex(const QJsonObject &obj, WonkoIndex *ptr);
	static void parseVersion(const QJsonObject &obj, WonkoVersion *ptr);
	static void parseVersionList(const QJsonObject &obj, WonkoVersionList *ptr);

	static QJsonObject serializeIndex(const WonkoIndex *ptr);
	static QJsonObject serializeVersion(const WonkoVersion *ptr);
	static QJsonObject serializeVersionList(const WonkoVersionList *ptr);

protected:
	virtual BaseWonkoEntity::Ptr parseIndexInternal(const QJsonObject &obj) const = 0;
	virtual BaseWonkoEntity::Ptr parseVersionInternal(const QJsonObject &obj) const = 0;
	virtual BaseWonkoEntity::Ptr parseVersionListInternal(const QJsonObject &obj) const = 0;
	virtual QJsonObject serializeIndexInternal(const WonkoIndex *ptr) const = 0;
	virtual QJsonObject serializeVersionInternal(const WonkoVersion *ptr) const = 0;
	virtual QJsonObject serializeVersionListInternal(const WonkoVersionList *ptr) const = 0;
};
