#include "ForgeInstaller.h"
#include "OneSixVersion.h"
#include "OneSixLibrary.h"
#include <quazip.h>
#include <quazipfile.h>
#include <pathutils.h>
#include <QStringList>

ForgeInstaller::ForgeInstaller(QString filename, QString universal_url)
{
	QSharedPointer<OneSixVersion> newVersion;
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
	}
	file.close();

	m_forge_version = newVersion;
	realVersionId = m_forge_version->id = installObj.value("minecraft").toString();
}

bool ForgeInstaller::apply(QSharedPointer<OneSixVersion> to)
{
	if (!m_forge_version)
		return false;
	to->externalUpdateStart();
	int sliding_insert_window = 0;
	{
		// for each library in the version we are adding (except for the blacklisted)
		QSet<QString> blacklist{"lwjgl", "lwjgl_util", "lwjgl-platform"};
		for (auto lib : m_forge_version->libraries)
		{
			QString libName = lib->name();
			// if this is the actual forge lib, set an absolute url for the download
			if(libName.contains("minecraftforge"))
			{
				lib->setAbsoluteUrl(m_universal_url);
			}
			if (blacklist.contains(libName))
				continue;

			// find an entry that matches this one
			bool found = false;
			for (auto tolib : to->libraries)
			{
				if (tolib->name() != libName)
					continue;
				found = true;
				// replace lib
				tolib = lib;
				break;
			}
			if (!found)
			{
				// add lib
				to->libraries.insert(sliding_insert_window, lib);
				sliding_insert_window++;
			}
		}
		to->mainClass = m_forge_version->mainClass;
		to->minecraftArguments = m_forge_version->minecraftArguments;
		to->processArguments = m_forge_version->processArguments;
	}
	to->externalUpdateFinish();
	return to->toOriginalFile();
}
