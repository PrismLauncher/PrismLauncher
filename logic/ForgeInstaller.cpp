/* Copyright 2013 MultiMC Contributors
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

#include "ForgeInstaller.h"
#include "OneSixVersion.h"
#include "OneSixLibrary.h"
#include "net/HttpMetaCache.h"
#include <quazip.h>
#include <quazipfile.h>
#include <pathutils.h>
#include <QStringList>
#include "MultiMC.h"
#include "OneSixInstance.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QSaveFile>
#include <QCryptographicHash>

ForgeInstaller::ForgeInstaller(QString filename, QString universal_url)
{
	std::shared_ptr<OneSixVersion> newVersion;
	m_universal_url = universal_url;

	QuaZip zip(filename);
	if (!zip.open(QuaZip::mdUnzip))
		return;

	QuaZipFile file(&zip);

	// read the install profile
	if (!zip.setCurrentFile("install_profile.json"))
		return;

	QJsonParseError jsonError;
	if (!file.open(QIODevice::ReadOnly))
		return;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(file.readAll(), &jsonError);
	file.close();
	if (jsonError.error != QJsonParseError::NoError)
		return;

	if (!jsonDoc.isObject())
		return;

	QJsonObject root = jsonDoc.object();

	auto installVal = root.value("install");
	auto versionInfoVal = root.value("versionInfo");
	if (!installVal.isObject() || !versionInfoVal.isObject())
		return;

	// read the forge version info
	{
		newVersion = OneSixVersion::fromJson(versionInfoVal.toObject());
		if (!newVersion)
			return;
	}

	QJsonObject installObj = installVal.toObject();
	QString libraryName = installObj.value("path").toString();
	internalPath = installObj.value("filePath").toString();

	// where do we put the library? decode the mojang path
	OneSixLibrary lib(libraryName);
	lib.finalize();

	auto cacheentry = MMC->metacache()->resolveEntry("libraries", lib.storagePath());
	finalPath = "libraries/" + lib.storagePath();
	if (!ensureFilePathExists(finalPath))
		return;

	if (!zip.setCurrentFile(internalPath))
		return;
	if (!file.open(QIODevice::ReadOnly))
		return;
	{
		QByteArray data = file.readAll();
		// extract file
		QSaveFile extraction(finalPath);
		if (!extraction.open(QIODevice::WriteOnly))
			return;
		if (extraction.write(data) != data.size())
			return;
		if (!extraction.commit())
			return;
		QCryptographicHash md5sum(QCryptographicHash::Md5);
		md5sum.addData(data);

		cacheentry->stale = false;
		cacheentry->md5sum = md5sum.result().toHex().constData();
		MMC->metacache()->updateEntry(cacheentry);
	}
	file.close();

	m_forge_version = newVersion;
	realVersionId = m_forge_version->id = installObj.value("minecraft").toString();
}

bool ForgeInstaller::add(OneSixInstance *to)
{
	if (!BaseInstaller::add(to))
	{
		return false;
	}

	QJsonObject obj;
	obj.insert("order", 5);

	if (!m_forge_version)
		return false;
	int sliding_insert_window = 0;
	{
		QJsonArray librariesPlus;

		// for each library in the version we are adding (except for the blacklisted)
		QSet<QString> blacklist{"lwjgl", "lwjgl_util", "lwjgl-platform"};
		for (auto lib : m_forge_version->libraries)
		{
			QString libName = lib->name();
			// WARNING: This could actually break.
			// if this is the actual forge lib, set an absolute url for the download
			if (libName.contains("minecraftforge"))
			{
				lib->setAbsoluteUrl(m_universal_url);
			}
			else if (libName.contains("scala"))
			{
				lib->setHint("forge-pack-xz");
			}
			if (blacklist.contains(libName))
				continue;

			QJsonObject libObj = lib->toJson();

			bool found = false;
			// find an entry that matches this one
			for (auto tolib : to->getFullVersion()->libraries)
			{
				if (tolib->name() != libName)
					continue;
				found = true;
				// replace lib
				libObj.insert("insert", QString("apply"));
				break;
			}
			if (!found)
			{
				// add lib
				QJsonObject insertObj;
				insertObj.insert("before", to->getFullVersion()->libraries.at(sliding_insert_window)->rawName());
				libObj.insert("insert", insertObj);
				sliding_insert_window++;
			}
			librariesPlus.append(libObj);
		}
		obj.insert("+libraries", librariesPlus);
		obj.insert("mainClass", m_forge_version->mainClass);
		obj.insert("minecraftArguments", m_forge_version->minecraftArguments);
		obj.insert("processArguments", m_forge_version->processArguments);
	}

	QFile file(filename(to->instanceRoot()));
	if (!file.open(QFile::WriteOnly))
	{
		QLOG_ERROR() << "Error opening" << file.fileName() << "for reading:" << file.errorString();
		return false;
	}
	file.write(QJsonDocument(obj).toJson());
	file.close();

	return true;
}
