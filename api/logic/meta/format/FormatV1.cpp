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

#include "FormatV1.h"
#include <minecraft/onesix/OneSixVersionFormat.h>

#include "Json.h"

#include "meta/Index.h"
#include "meta/Version.h"
#include "meta/VersionList.h"
#include "Env.h"

using namespace Json;

namespace Meta
{

static const int currentFormatVersion = 0;

// Index

BaseEntity::Ptr FormatV1::parseIndexInternal(const QJsonObject &obj) const
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

QJsonObject FormatV1::serializeIndexInternal(const Index *ptr) const
{
	QJsonArray packages;
	for (const VersionListPtr &list : ptr->lists())
	{
		QJsonObject out;
		out["uid"] = list->uid();
		out["name"] = list->name();
		packages.append(out);
	}
	QJsonObject out;
	out["formatVersion"] = currentFormatVersion;
	out["packages"] = packages;
	return out;
}


// Version

static VersionPtr parseCommonVersion(const QString &uid, const QJsonObject &obj)
{
	const QVector<QJsonObject> requiresRaw = obj.contains("requires") ? requireIsArrayOf<QJsonObject>(obj, "requires") : QVector<QJsonObject>();
	QVector<Reference> requires;
	requires.reserve(requiresRaw.size());
	std::transform(requiresRaw.begin(), requiresRaw.end(), std::back_inserter(requires), [](const QJsonObject &rObj)
	{
		Reference ref(requireString(rObj, "uid"));
		ref.setVersion(ensureString(rObj, "version", QString()));
		return ref;
	});

	VersionPtr version = std::make_shared<Version>(uid, requireString(obj, "version"));
	version->setTime(QDateTime::fromString(requireString(obj, "releaseTime"), Qt::ISODate).toMSecsSinceEpoch() / 1000);
	version->setType(ensureString(obj, "type", QString()));
	version->setRequires(requires);
	return version;
}

BaseEntity::Ptr FormatV1::parseVersionInternal(const QJsonObject &obj) const
{
	VersionPtr version = parseCommonVersion(requireString(obj, "uid"), obj);

	version->setData(OneSixVersionFormat::versionFileFromJson(QJsonDocument(obj),
										   QString("%1/%2.json").arg(version->uid(), version->version()),
										   obj.contains("order")));
	return version;
}

static void serializeCommonVersion(const Version *version, QJsonObject &obj)
{
	QJsonArray requires;
	for (const Reference &ref : version->requires())
	{
		if (ref.version().isEmpty())
		{
			QJsonObject out;
			out["uid"] = ref.uid();
			requires.append(out);
		}
		else
		{
			QJsonObject out;
			out["uid"] = ref.uid();
			out["version"] = ref.version();
			requires.append(out);
		}
	}

	obj.insert("version", version->version());
	obj.insert("type", version->type());
	obj.insert("releaseTime", version->time().toString(Qt::ISODate));
	obj.insert("requires", requires);
}

QJsonObject FormatV1::serializeVersionInternal(const Version *ptr) const
{
	QJsonObject obj = OneSixVersionFormat::versionFileToJson(ptr->data(), true).object();
	serializeCommonVersion(ptr, obj);
	obj.insert("formatVersion", currentFormatVersion);
	obj.insert("uid", ptr->uid());
	// TODO: the name should be looked up in the UI based on the uid
	obj.insert("name", ENV.metadataIndex()->getListGuaranteed(ptr->uid())->name());

	return obj;
}


// Version list / package

BaseEntity::Ptr FormatV1::parseVersionListInternal(const QJsonObject &obj) const
{
	const QString uid = requireString(obj, "uid");

	const QVector<QJsonObject> versionsRaw = requireIsArrayOf<QJsonObject>(obj, "versions");
	QVector<VersionPtr> versions;
	versions.reserve(versionsRaw.size());
	std::transform(versionsRaw.begin(), versionsRaw.end(), std::back_inserter(versions), [this, uid](const QJsonObject &vObj)
	{ return parseCommonVersion(uid, vObj); });

	VersionListPtr list = std::make_shared<VersionList>(uid);
	list->setName(ensureString(obj, "name", QString()));
	list->setVersions(versions);
	return list;
}

QJsonObject FormatV1::serializeVersionListInternal(const VersionList *ptr) const
{
	QJsonArray versions;
	for (const VersionPtr &version : ptr->versions())
	{
		QJsonObject obj;
		serializeCommonVersion(version.get(), obj);
		versions.append(obj);
	}
	QJsonObject out;
	out["formatVersion"] = currentFormatVersion;
	out["uid"] = ptr->uid();
	out["name"] = ptr->name().isNull() ? QJsonValue() : ptr->name();
	out["versions"] = versions;
	return out;
}
}
