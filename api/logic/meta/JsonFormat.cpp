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

#include "JsonFormat.h"

// FIXME: remove this from here... somehow
#include "minecraft/OneSixVersionFormat.h"
#include "Json.h"

#include "Index.h"
#include "Version.h"
#include "VersionList.h"

using namespace Json;

namespace Meta
{

// Index
static BaseEntity::Ptr parseIndexInternal(const QJsonObject &obj)
{
	const QVector<QJsonObject> objects = requireIsArrayOf<QJsonObject>(obj, "packages");
	QVector<VersionListPtr> lists;
	lists.reserve(objects.size());
	std::transform(objects.begin(), objects.end(), std::back_inserter(lists), [](const QJsonObject &obj)
	{
		VersionListPtr list = std::make_shared<VersionList>(requireString(obj, "uid"));
		list->setName(ensureString(obj, "name", QString()));
		return list;
	});
	return std::make_shared<Index>(lists);
}

// Version
static VersionPtr parseCommonVersion(const QString &uid, const QJsonObject &obj)
{
	VersionPtr version = std::make_shared<Version>(uid, requireString(obj, "version"));
	version->setTime(QDateTime::fromString(requireString(obj, "releaseTime"), Qt::ISODate).toMSecsSinceEpoch() / 1000);
	version->setType(ensureString(obj, "type", QString()));
	version->setParentUid(ensureString(obj, "parentUid", QString()));
	version->setRecommended(ensureBoolean(obj, QString("recommended"), false));
	if(obj.contains("requires"))
	{
		QHash<QString, QString> requires;
		auto reqobj = requireObject(obj, "requires");
		auto iter = reqobj.begin();
		while(iter != reqobj.end())
		{
			requires[iter.key()] = requireString(iter.value());
			iter++;
		}
		version->setRequires(requires);
	}
	return version;
}

static BaseEntity::Ptr parseVersionInternal(const QJsonObject &obj)
{
	VersionPtr version = parseCommonVersion(requireString(obj, "uid"), obj);

	version->setData(OneSixVersionFormat::versionFileFromJson(QJsonDocument(obj),
										   QString("%1/%2.json").arg(version->uid(), version->version()),
										   obj.contains("order")));
	return version;
}

// Version list / package
static BaseEntity::Ptr parseVersionListInternal(const QJsonObject &obj)
{
	const QString uid = requireString(obj, "uid");

	const QVector<QJsonObject> versionsRaw = requireIsArrayOf<QJsonObject>(obj, "versions");
	QVector<VersionPtr> versions;
	versions.reserve(versionsRaw.size());
	std::transform(versionsRaw.begin(), versionsRaw.end(), std::back_inserter(versions), [uid](const QJsonObject &vObj)
	{
		auto version = parseCommonVersion(uid, vObj);
		version->setProvidesRecommendations();
		return version;
	});

	VersionListPtr list = std::make_shared<VersionList>(uid);
	list->setName(ensureString(obj, "name", QString()));
	list->setParentUid(ensureString(obj, "parentUid", QString()));
	list->setVersions(versions);
	return list;
}


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

void parseIndex(const QJsonObject &obj, Index *ptr)
{
	const int version = formatVersion(obj);
	switch (version) {
	case 0:
		ptr->merge(parseIndexInternal(obj));
		break;
	default:
		throw ParseException(QObject::tr("Unknown formatVersion: %1").arg(version));
	}
}

void parseVersionList(const QJsonObject &obj, VersionList *ptr)
{
	const int version = formatVersion(obj);
	switch (version) {
	case 0:
		ptr->merge(parseVersionListInternal(obj));
		break;
	default:
		throw ParseException(QObject::tr("Unknown formatVersion: %1").arg(version));
	}
}

void parseVersion(const QJsonObject &obj, Version *ptr)
{
	const int version = formatVersion(obj);
	switch (version) {
	case 0:
		ptr->merge(parseVersionInternal(obj));
		break;
	default:
		throw ParseException(QObject::tr("Unknown formatVersion: %1").arg(version));
	}
}
}
