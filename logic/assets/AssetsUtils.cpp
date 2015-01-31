/* Copyright 2013-2015 MultiMC Contributors
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
#include <QVariant>
#include <logger/QsLog.h>

#include "AssetsUtils.h"
#include <pathutils.h>

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

QDir reconstructAssets(QString assetsId)
{
	QDir assetsDir = QDir("assets/");
	QDir indexDir = QDir(PathCombine(assetsDir.path(), "indexes"));
	QDir objectDir = QDir(PathCombine(assetsDir.path(), "objects"));
	QDir virtualDir = QDir(PathCombine(assetsDir.path(), "virtual"));

	QString indexPath = PathCombine(indexDir.path(), assetsId + ".json");
	QFile indexFile(indexPath);
	QDir virtualRoot(PathCombine(virtualDir.path(), assetsId));

	if (!indexFile.exists())
	{
		QLOG_ERROR() << "No assets index file" << indexPath << "; can't reconstruct assets";
		return virtualRoot;
	}

	QLOG_DEBUG() << "reconstructAssets" << assetsDir.path() << indexDir.path()
				 << objectDir.path() << virtualDir.path() << virtualRoot.path();

	AssetsIndex index;
	bool loadAssetsIndex = AssetsUtils::loadAssetsIndexJson(indexPath, &index);

	if (loadAssetsIndex && index.isVirtual)
	{
		QLOG_INFO() << "Reconstructing virtual assets folder at" << virtualRoot.path();

		for (QString map : index.objects.keys())
		{
			AssetObject asset_object = index.objects.value(map);
			QString target_path = PathCombine(virtualRoot.path(), map);
			QFile target(target_path);

			QString tlk = asset_object.hash.left(2);

			QString original_path =
				PathCombine(PathCombine(objectDir.path(), tlk), asset_object.hash);
			QFile original(original_path);
			if (!original.exists())
				continue;
			if (!target.exists())
			{
				QFileInfo info(target_path);
				QDir target_dir = info.dir();
				// QLOG_DEBUG() << target_dir;
				if (!target_dir.exists())
					QDir("").mkpath(target_dir.path());

				bool couldCopy = original.copy(target_path);
				QLOG_DEBUG() << " Copying" << original_path << "to" << target_path
							 << QString::number(couldCopy); // << original.errorString();
			}
		}

		// TODO: Write last used time to virtualRoot/.lastused
	}

	return virtualRoot;
}

}
