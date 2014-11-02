/* Copyright 2013-2014 MultiMC Contributors
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

#include <QDir>
#include <QDirIterator>
#include <QCryptographicHash>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>

#include "AssetsUtils.h"
#include "MultiMC.h"

namespace AssetsUtils
{
int findLegacyAssets()
{
	QDir assets_dir("assets");
	if (!assets_dir.exists())
		return 0;
	assets_dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
	int base_length = assets_dir.path().length();

	QList<QString> blacklist = {"indexes", "objects", "virtual"};

	QDirIterator iterator(assets_dir, QDirIterator::Subdirectories);
	int found = 0;
	while (iterator.hasNext())
	{
		QString currentDir = iterator.next();
		currentDir = currentDir.remove(0, base_length + 1);

		bool ignore = false;
		for (QString blacklisted : blacklist)
		{
			if (currentDir.startsWith(blacklisted))
				ignore = true;
		}

		if (!iterator.fileInfo().isDir() && !ignore)
		{
			found++;
		}
	}

	return found;
}

/*
 * Returns true on success, with index populated
 * index is undefined otherwise
 */
bool loadAssetsIndexJson(QString path, AssetsIndex *index)
{
	/*
	{
	  "objects": {
		"icons/icon_16x16.png": {
		  "hash": "bdf48ef6b5d0d23bbb02e17d04865216179f510a",
		  "size": 3665
		},
		...
		}
	  }
	}
	*/

	QFile file(path);

	// Try to open the file and fail if we can't.
	// TODO: We should probably report this error to the user.
	if (!file.open(QIODevice::ReadOnly))
	{
		QLOG_ERROR() << "Failed to read assets index file" << path;
		return false;
	}

	// Read the file and close it.
	QByteArray jsonData = file.readAll();
	file.close();

	QJsonParseError parseError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);

	// Fail if the JSON is invalid.
	if (parseError.error != QJsonParseError::NoError)
	{
		QLOG_ERROR() << "Failed to parse assets index file:" << parseError.errorString()
					 << "at offset " << QString::number(parseError.offset);
		return false;
	}

	// Make sure the root is an object.
	if (!jsonDoc.isObject())
	{
		QLOG_ERROR() << "Invalid assets index JSON: Root should be an array.";
		return false;
	}

	QJsonObject root = jsonDoc.object();

	QJsonValue isVirtual = root.value("virtual");
	if (!isVirtual.isUndefined())
	{
		index->isVirtual = isVirtual.toBool(false);
	}

	QJsonValue objects = root.value("objects");
	QVariantMap map = objects.toVariant().toMap();

	for (QVariantMap::const_iterator iter = map.begin(); iter != map.end(); ++iter)
	{
		// QLOG_DEBUG() << iter.key();

		QVariant variant = iter.value();
		QVariantMap nested_objects = variant.toMap();

		AssetObject object;

		for (QVariantMap::const_iterator nested_iter = nested_objects.begin();
			 nested_iter != nested_objects.end(); ++nested_iter)
		{
			// QLOG_DEBUG() << nested_iter.key() << nested_iter.value().toString();
			QString key = nested_iter.key();
			QVariant value = nested_iter.value();

			if (key == "hash")
			{
				object.hash = value.toString();
			}
			else if (key == "size")
			{
				object.size = value.toDouble();
			}
		}

		index->objects.insert(iter.key(), object);
	}

	return true;
}
}
