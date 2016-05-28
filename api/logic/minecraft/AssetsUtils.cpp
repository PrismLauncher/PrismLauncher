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

#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QCryptographicHash>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariant>
#include <QDebug>

#include "AssetsUtils.h"
#include "FileSystem.h"
#include "net/Download.h"
#include "net/ChecksumValidator.h"


namespace AssetsUtils
{

/*
 * Returns true on success, with index populated
 * index is undefined otherwise
 */
bool loadAssetsIndexJson(QString assetsId, QString path, AssetsIndex *index)
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
		qCritical() << "Failed to read assets index file" << path;
		return false;
	}
	index->id = assetsId;

	// Read the file and close it.
	QByteArray jsonData = file.readAll();
	file.close();

	QJsonParseError parseError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);

	// Fail if the JSON is invalid.
	if (parseError.error != QJsonParseError::NoError)
	{
		qCritical() << "Failed to parse assets index file:" << parseError.errorString()
					 << "at offset " << QString::number(parseError.offset);
		return false;
	}

	// Make sure the root is an object.
	if (!jsonDoc.isObject())
	{
		qCritical() << "Invalid assets index JSON: Root should be an array.";
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
		// qDebug() << iter.key();

		QVariant variant = iter.value();
		QVariantMap nested_objects = variant.toMap();

		AssetObject object;

		for (QVariantMap::const_iterator nested_iter = nested_objects.begin();
			 nested_iter != nested_objects.end(); ++nested_iter)
		{
			// qDebug() << nested_iter.key() << nested_iter.value().toString();
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
	QDir indexDir = QDir(FS::PathCombine(assetsDir.path(), "indexes"));
	QDir objectDir = QDir(FS::PathCombine(assetsDir.path(), "objects"));
	QDir virtualDir = QDir(FS::PathCombine(assetsDir.path(), "virtual"));

	QString indexPath = FS::PathCombine(indexDir.path(), assetsId + ".json");
	QFile indexFile(indexPath);
	QDir virtualRoot(FS::PathCombine(virtualDir.path(), assetsId));

	if (!indexFile.exists())
	{
		qCritical() << "No assets index file" << indexPath << "; can't reconstruct assets";
		return virtualRoot;
	}

	qDebug() << "reconstructAssets" << assetsDir.path() << indexDir.path()
				 << objectDir.path() << virtualDir.path() << virtualRoot.path();

	AssetsIndex index;
	bool loadAssetsIndex = AssetsUtils::loadAssetsIndexJson(assetsId, indexPath, &index);

	if (loadAssetsIndex && index.isVirtual)
	{
		qDebug() << "Reconstructing virtual assets folder at" << virtualRoot.path();

		for (QString map : index.objects.keys())
		{
			AssetObject asset_object = index.objects.value(map);
			QString target_path = FS::PathCombine(virtualRoot.path(), map);
			QFile target(target_path);

			QString tlk = asset_object.hash.left(2);

			QString original_path = FS::PathCombine(objectDir.path(), tlk, asset_object.hash);
			QFile original(original_path);
			if (!original.exists())
				continue;
			if (!target.exists())
			{
				QFileInfo info(target_path);
				QDir target_dir = info.dir();
				// qDebug() << target_dir;
				if (!target_dir.exists())
					QDir("").mkpath(target_dir.path());

				bool couldCopy = original.copy(target_path);
				qDebug() << " Copying" << original_path << "to" << target_path
							 << QString::number(couldCopy); // << original.errorString();
			}
		}

		// TODO: Write last used time to virtualRoot/.lastused
	}

	return virtualRoot;
}

}

NetActionPtr AssetObject::getDownloadAction()
{
	QFileInfo objectFile(getLocalPath());
	if ((!objectFile.isFile()) || (objectFile.size() != size))
	{
		auto objectDL = Net::Download::makeFile(getUrl(), objectFile.filePath());
		if(hash.size())
		{
			auto rawHash = QByteArray::fromHex(hash.toLatin1());
			objectDL->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha1, rawHash));
		}
		objectDL->m_total_progress = size;
		return objectDL;
	}
	return nullptr;
}

QString AssetObject::getLocalPath()
{
	return "assets/objects/" + getRelPath();
}

QUrl AssetObject::getUrl()
{
	return QUrl("http://resources.download.minecraft.net/" + getRelPath());
}

QString AssetObject::getRelPath()
{
	return hash.left(2) + "/" + hash;
}

NetJobPtr AssetsIndex::getDownloadJob()
{
	auto job = new NetJob(QObject::tr("Assets for %1").arg(id));
	for (auto &object : objects.values())
	{
		auto dl = object.getDownloadAction();
		if(dl)
		{
			job->addNetAction(dl);
		}
	}
	if(job->size())
		return job;
	return nullptr;
}
