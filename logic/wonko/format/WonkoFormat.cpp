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

#include "WonkoFormat.h"

#include "WonkoFormatV1.h"

#include "wonko/WonkoIndex.h"
#include "wonko/WonkoVersion.h"
#include "wonko/WonkoVersionList.h"

static int formatVersion(const QJsonObject &obj)
{
	if (!obj.contains("formatVersion")) {
		throw WonkoParseException(QObject::tr("Missing required field: 'formatVersion'"));
	}
	if (!obj.value("formatVersion").isDouble()) {
		throw WonkoParseException(QObject::tr("Required field has invalid type: 'formatVersion'"));
	}
	return obj.value("formatVersion").toInt();
}

void WonkoFormat::parseIndex(const QJsonObject &obj, WonkoIndex *ptr)
{
	const int version = formatVersion(obj);
	switch (version) {
	case 1:
		ptr->merge(WonkoFormatV1().parseIndexInternal(obj));
		break;
	default:
		throw WonkoParseException(QObject::tr("Unknown formatVersion: %1").arg(version));
	}
}
void WonkoFormat::parseVersion(const QJsonObject &obj, WonkoVersion *ptr)
{
	const int version = formatVersion(obj);
	switch (version) {
	case 1:
		ptr->merge(WonkoFormatV1().parseVersionInternal(obj));
		break;
	default:
		throw WonkoParseException(QObject::tr("Unknown formatVersion: %1").arg(version));
	}
}
void WonkoFormat::parseVersionList(const QJsonObject &obj, WonkoVersionList *ptr)
{
	const int version = formatVersion(obj);
	switch (version) {
	case 10:
		ptr->merge(WonkoFormatV1().parseVersionListInternal(obj));
		break;
	default:
		throw WonkoParseException(QObject::tr("Unknown formatVersion: %1").arg(version));
	}
}

QJsonObject WonkoFormat::serializeIndex(const WonkoIndex *ptr)
{
	return WonkoFormatV1().serializeIndexInternal(ptr);
}
QJsonObject WonkoFormat::serializeVersion(const WonkoVersion *ptr)
{
	return WonkoFormatV1().serializeVersionInternal(ptr);
}
QJsonObject WonkoFormat::serializeVersionList(const WonkoVersionList *ptr)
{
	return WonkoFormatV1().serializeVersionListInternal(ptr);
}
