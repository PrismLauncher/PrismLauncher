/* Copyright 2015-2017 MultiMC Contributors
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

#include "Format.h"

#include "FormatV1.h"

#include "meta/Index.h"
#include "meta/Version.h"
#include "meta/VersionList.h"

namespace Meta
{

static int formatVersion(const QJsonObject &obj)
{
	if (!obj.contains("formatVersion")) {
		throw ParseException(QObject::tr("Missing required field: 'formatVersion'"));
	}
	if (!obj.value("formatVersion").isDouble()) {
		throw ParseException(QObject::tr("Required field has invalid type: 'formatVersion'"));
	}
	return obj.value("formatVersion").toInt();
}

void Format::parseIndex(const QJsonObject &obj, Index *ptr)
{
	const int version = formatVersion(obj);
	switch (version) {
	case 1:
		ptr->merge(FormatV1().parseIndexInternal(obj));
		break;
	default:
		throw ParseException(QObject::tr("Unknown formatVersion: %1").arg(version));
	}
}
void Format::parseVersion(const QJsonObject &obj, Version *ptr)
{
	const int version = formatVersion(obj);
	switch (version) {
	case 1:
		ptr->merge(FormatV1().parseVersionInternal(obj));
		break;
	default:
		throw ParseException(QObject::tr("Unknown formatVersion: %1").arg(version));
	}
}
void Format::parseVersionList(const QJsonObject &obj, VersionList *ptr)
{
	const int version = formatVersion(obj);
	switch (version) {
	case 10:
		ptr->merge(FormatV1().parseVersionListInternal(obj));
		break;
	default:
		throw ParseException(QObject::tr("Unknown formatVersion: %1").arg(version));
	}
}

QJsonObject Format::serializeIndex(const Index *ptr)
{
	return FormatV1().serializeIndexInternal(ptr);
}
QJsonObject Format::serializeVersion(const Version *ptr)
{
	return FormatV1().serializeVersionInternal(ptr);
}
QJsonObject Format::serializeVersionList(const VersionList *ptr)
{
	return FormatV1().serializeVersionListInternal(ptr);
}
}
