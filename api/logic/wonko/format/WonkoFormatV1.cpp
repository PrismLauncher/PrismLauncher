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

#include "WonkoFormatV1.h"
#include <minecraft/onesix/OneSixVersionFormat.h>

#include "Json.h"

#include "wonko/WonkoIndex.h"
#include "wonko/WonkoVersion.h"
#include "wonko/WonkoVersionList.h"
#include "Env.h"

using namespace Json;

static WonkoVersionPtr parseCommonVersion(const QString &uid, const QJsonObject &obj)
{
	const QVector<QJsonObject> requiresRaw = obj.contains("requires") ? requireIsArrayOf<QJsonObject>(obj, "requires") : QVector<QJsonObject>();
	QVector<WonkoReference> requires;
	requires.reserve(requiresRaw.size());
	std::transform(requiresRaw.begin(), requiresRaw.end(), std::back_inserter(requires), [](const QJsonObject &rObj)
	{
		WonkoReference ref(requireString(rObj, "uid"));
		ref.setVersion(ensureString(rObj, "version", QString()));
		return ref;
	});

	WonkoVersionPtr version = std::make_shared<WonkoVersion>(uid, requireString(obj, "version"));
	if (obj.value("time").isString())
	{
		version->setTime(QDateTime::fromString(requireString(obj, "time"), Qt::ISODate).toMSecsSinceEpoch() / 1000);
	}
	else
	{
		version->setTime(requireInteger(obj, "time"));
	}
	version->setType(ensureString(obj, "type", QString()));
	version->setRequires(requires);
	return version;
}
static void serializeCommonVersion(const WonkoVersion *version, QJsonObject &obj)
{
	QJsonArray requires;
	for (const WonkoReference &ref : version->requires())
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
	obj.insert("time", version->time().toString(Qt::ISODate));
	obj.insert("requires", requires);
}

BaseWonkoEntity::Ptr WonkoFormatV1::parseIndexInternal(const QJsonObject &obj) const
{
	const QVector<QJsonObject> objects = requireIsArrayOf<QJsonObject>(obj, "index");
	QVector<WonkoVersionListPtr> lists;
	lists.reserve(objects.size());
	std::transform(objects.begin(), objects.end(), std::back_inserter(lists), [](const QJsonObject &obj)
	{
		WonkoVersionListPtr list = std::make_shared<WonkoVersionList>(requireString(obj, "uid"));
		list->setName(ensureString(obj, "name", QString()));
		return list;
	});
	return std::make_shared<WonkoIndex>(lists);
}
BaseWonkoEntity::Ptr WonkoFormatV1::parseVersionInternal(const QJsonObject &obj) const
{
	WonkoVersionPtr version = parseCommonVersion(requireString(obj, "uid"), obj);

	version->setData(OneSixVersionFormat::versionFileFromJson(QJsonDocument(obj),
										   QString("%1/%2.json").arg(version->uid(), version->version()),
										   obj.contains("order")));
	return version;
}
BaseWonkoEntity::Ptr WonkoFormatV1::parseVersionListInternal(const QJsonObject &obj) const
{
	const QString uid = requireString(obj, "uid");

	const QVector<QJsonObject> versionsRaw = requireIsArrayOf<QJsonObject>(obj, "versions");
	QVector<WonkoVersionPtr> versions;
	versions.reserve(versionsRaw.size());
	std::transform(versionsRaw.begin(), versionsRaw.end(), std::back_inserter(versions), [this, uid](const QJsonObject &vObj)
	{ return parseCommonVersion(uid, vObj); });

	WonkoVersionListPtr list = std::make_shared<WonkoVersionList>(uid);
	list->setName(ensureString(obj, "name", QString()));
	list->setVersions(versions);
	return list;
}

QJsonObject WonkoFormatV1::serializeIndexInternal(const WonkoIndex *ptr) const
{
	QJsonArray index;
	for (const WonkoVersionListPtr &list : ptr->lists())
	{
		QJsonObject out;
		out["uid"] = list->uid();
		out["version"] = list->name();
		index.append(out);
	}
	QJsonObject out;
	out["formatVersion"] = 1;
	out["index"] = index;
	return out;
}
QJsonObject WonkoFormatV1::serializeVersionInternal(const WonkoVersion *ptr) const
{
	QJsonObject obj = OneSixVersionFormat::versionFileToJson(ptr->data(), true).object();
	serializeCommonVersion(ptr, obj);
	obj.insert("formatVersion", 1);
	obj.insert("uid", ptr->uid());
	// TODO: the name should be looked up in the UI based on the uid
	obj.insert("name", ENV.wonkoIndex()->getListGuaranteed(ptr->uid())->name());

	return obj;
}
QJsonObject WonkoFormatV1::serializeVersionListInternal(const WonkoVersionList *ptr) const
{
	QJsonArray versions;
	for (const WonkoVersionPtr &version : ptr->versions())
	{
		QJsonObject obj;
		serializeCommonVersion(version.get(), obj);
		versions.append(obj);
	}
	QJsonObject out;
	out["formatVersion"] = 10;
	out["uid"] = ptr->uid();
	out["name"] = ptr->name().isNull() ? QJsonValue() : ptr->name();
	out["versions"] = versions;
	return out;
}
