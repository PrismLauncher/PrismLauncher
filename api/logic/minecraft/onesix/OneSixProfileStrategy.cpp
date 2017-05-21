#include "OneSixProfileStrategy.h"
#include "OneSixInstance.h"
#include "OneSixVersionFormat.h"

#include "Env.h"
#include <FileSystem.h>

#include <QDir>
#include <QUuid>
#include <QJsonDocument>
#include <QJsonArray>
#include <QSaveFile>
#include <QResource>
#include <meta/Index.h>
#include <meta/Version.h>

#include <tuple>

OneSixProfileStrategy::OneSixProfileStrategy(OneSixInstance* instance)
{
	m_instance = instance;
}

void OneSixProfileStrategy::upgradeDeprecatedFiles()
{
	auto versionJsonPath = FS::PathCombine(m_instance->instanceRoot(), "version.json");
	auto customJsonPath = FS::PathCombine(m_instance->instanceRoot(), "custom.json");
	auto mcJson = FS::PathCombine(m_instance->instanceRoot(), "patches" , "net.minecraft.json");

	QString sourceFile;
	QString renameFile;

	// convert old crap.
	if(QFile::exists(customJsonPath))
	{
		sourceFile = customJsonPath;
		renameFile = versionJsonPath;
	}
	else if(QFile::exists(versionJsonPath))
	{
		sourceFile = versionJsonPath;
	}
	if(!sourceFile.isEmpty() && !QFile::exists(mcJson))
	{
		if(!FS::ensureFilePathExists(mcJson))
		{
			qWarning() << "Couldn't create patches folder for" << m_instance->name();
			return;
		}
		if(!renameFile.isEmpty() && QFile::exists(renameFile))
		{
			if(!QFile::rename(renameFile, renameFile + ".old"))
			{
				qWarning() << "Couldn't rename" << renameFile << "to" << renameFile + ".old" << "in" << m_instance->name();
				return;
			}
		}
		auto file = ProfileUtils::parseJsonFile(QFileInfo(sourceFile), false);
		ProfileUtils::removeLwjglFromPatch(file);
		file->uid = "net.minecraft";
		file->version = file->minecraftVersion;
		file->name = "Minecraft";
		auto data = OneSixVersionFormat::versionFileToJson(file, false).toJson();
		QSaveFile newPatchFile(mcJson);
		if(!newPatchFile.open(QIODevice::WriteOnly))
		{
			newPatchFile.cancelWriting();
			qWarning() << "Couldn't open main patch for writing in" << m_instance->name();
			return;
		}
		newPatchFile.write(data);
		if(!newPatchFile.commit())
		{
			qWarning() << "Couldn't save main patch in" << m_instance->name();
			return;
		}
		if(!QFile::rename(sourceFile, sourceFile + ".old"))
		{
			qWarning() << "Couldn't rename" << sourceFile << "to" << sourceFile + ".old" << "in" << m_instance->name();
			return;
		}
	}
}

void OneSixProfileStrategy::loadDefaultBuiltinPatches()
{
	auto addBuiltinPatch = [&](const QString &uid, const QString intendedVersion, int order)
	{
		auto jsonFilePath = FS::PathCombine(m_instance->instanceRoot(), "patches" , uid + ".json");
		// load up the base minecraft patch
		ProfilePatchPtr profilePatch;
		if(QFile::exists(jsonFilePath))
		{
			auto file = ProfileUtils::parseJsonFile(QFileInfo(jsonFilePath), false);
			if(file->version.isEmpty())
			{
				file->version = intendedVersion;
			}
			profilePatch = std::make_shared<ProfilePatch>(file, jsonFilePath);
			profilePatch->setVanilla(false);
			profilePatch->setRevertible(true);
		}
		else
		{
			auto metaVersion = ENV.metadataIndex()->get(uid, intendedVersion);
			profilePatch = std::make_shared<ProfilePatch>(metaVersion);
			profilePatch->setVanilla(true);
		}
		profilePatch->setOrder(order);
		profile->appendPatch(profilePatch);
	};
	addBuiltinPatch("net.minecraft", m_instance->getComponentVersion("net.minecraft"), -2);
	addBuiltinPatch("org.lwjgl", m_instance->getComponentVersion("org.lwjgl"), -1);
}

void OneSixProfileStrategy::loadUserPatches()
{
	// first, collect all patches (that are not builtins of OneSix) and load them
	QMap<QString, ProfilePatchPtr> loadedPatches;
	QDir patchesDir(FS::PathCombine(m_instance->instanceRoot(),"patches"));
	for (auto info : patchesDir.entryInfoList(QStringList() << "*.json", QDir::Files))
	{
		// parse the file
		qDebug() << "Reading" << info.fileName();
		auto file = ProfileUtils::parseJsonFile(info, true);
		// ignore builtins
		if (file->uid == "net.minecraft")
			continue;
		if (file->uid == "org.lwjgl")
			continue;
		auto patch = std::make_shared<ProfilePatch>(file, info.filePath());
		patch->setRemovable(true);
		patch->setMovable(true);
		if(ENV.metadataIndex()->hasUid(file->uid))
		{
			// FIXME: requesting a uid/list creates it in the index... this allows reverting to possibly invalid versions...
			patch->setRevertible(true);
		}
		loadedPatches[file->uid] = patch;
	}
	// these are 'special'... if not already loaded from instance files, grab them from the metadata repo.
	auto loadSpecial = [&](const QString & uid, int order)
	{
		auto patchVersion = m_instance->getComponentVersion(uid);
		if(!patchVersion.isEmpty() && !loadedPatches.contains(uid))
		{
			auto patch = std::make_shared<ProfilePatch>(ENV.metadataIndex()->get(uid, patchVersion));
			patch->setOrder(order);
			patch->setVanilla(true);
			patch->setRemovable(true);
			patch->setMovable(true);
			loadedPatches[uid] = patch;
		}
	};
	loadSpecial("net.minecraftforge", 5);
	loadSpecial("com.mumfrey.liteloader", 10);

	// now add all the patches by user sort order
	ProfileUtils::PatchOrder userOrder;
	ProfileUtils::readOverrideOrders(FS::PathCombine(m_instance->instanceRoot(), "order.json"), userOrder);
	for (auto uid : userOrder)
	{
		// ignore builtins
		if (uid == "net.minecraft")
			continue;
		if (uid == "org.lwjgl")
			continue;
		// ordering has a patch that is gone?
		if(!loadedPatches.contains(uid))
		{
			continue;
		}
		profile->appendPatch(loadedPatches.take(uid));
	}

	// is there anything left to sort?
	if(loadedPatches.isEmpty())
	{
		// TODO: save the order here?
		return;
	}

	// inserting into multimap by order number as key sorts the patches and detects duplicates
	QMultiMap<int, ProfilePatchPtr> files;
	auto iter = loadedPatches.begin();
	while(iter != loadedPatches.end())
	{
		files.insert((*iter)->getOrder(), *iter);
		iter++;
	}

	// then just extract the patches and put them in the list
	for (auto order : files.keys())
	{
		const auto &values = files.values(order);
		for(auto &value: values)
		{
			// TODO: put back the insertion of problem messages here, so the user knows about the id duplication
			profile->appendPatch(value);
		}
	}
	// TODO: save the order here?
}


void OneSixProfileStrategy::load()
{
	profile->clearPatches();

	upgradeDeprecatedFiles();
	loadDefaultBuiltinPatches();
	loadUserPatches();
}

bool OneSixProfileStrategy::saveOrder(ProfileUtils::PatchOrder order)
{
	return ProfileUtils::writeOverrideOrders(FS::PathCombine(m_instance->instanceRoot(), "order.json"), order);
}

bool OneSixProfileStrategy::resetOrder()
{
	return QDir(m_instance->instanceRoot()).remove("order.json");
}

bool OneSixProfileStrategy::removePatch(ProfilePatchPtr patch)
{
	bool ok = true;
	// first, remove the patch file. this ensures it's not used anymore
	auto fileName = patch->getFilename();
	if(fileName.size())
	{
		QFile patchFile(fileName);
		if(patchFile.exists() && !patchFile.remove())
		{
			qCritical() << "File" << fileName << "could not be removed because:" << patchFile.errorString();
			return false;
		}
	}
	if(!m_instance->getComponentVersion(patch->getID()).isEmpty())
	{
		m_instance->setComponentVersion(patch->getID(), QString());
	}

	// FIXME: we need a generic way of removing local resources, not just jar mods...
	auto preRemoveJarMod = [&](LibraryPtr jarMod) -> bool
	{
		if (!jarMod->isLocal())
		{
			return true;
		}
		QStringList jar, temp1, temp2, temp3;
		jarMod->getApplicableFiles(currentSystem, jar, temp1, temp2, temp3, m_instance->jarmodsPath().absolutePath());
		QFileInfo finfo (jar[0]);
		if(finfo.exists())
		{
			QFile jarModFile(jar[0]);
			if(!jarModFile.remove())
			{
				qCritical() << "File" << jar[0] << "could not be removed because:" << jarModFile.errorString();
				return false;
			}
			return true;
		}
		return true;
	};

	auto &jarMods = patch->getVersionFile()->jarMods;
	for(auto &jarmod: jarMods)
	{
		ok &= preRemoveJarMod(jarmod);
	}
	return ok;
}

bool OneSixProfileStrategy::customizePatch(ProfilePatchPtr patch)
{
	if(patch->isCustom())
	{
		return false;
	}

	auto filename = FS::PathCombine(m_instance->instanceRoot(), "patches" , patch->getID() + ".json");
	if(!FS::ensureFilePathExists(filename))
	{
		return false;
	}
	// FIXME: get rid of this try-catch.
	try
	{
		QSaveFile jsonFile(filename);
		if(!jsonFile.open(QIODevice::WriteOnly))
		{
			return false;
		}
		auto vfile = patch->getVersionFile();
		if(!vfile)
		{
			return false;
		}
		auto document = OneSixVersionFormat::versionFileToJson(vfile, true);
		jsonFile.write(document.toJson());
		if(!jsonFile.commit())
		{
			return false;
		}
		load();
	}
	catch (Exception &error)
	{
		qWarning() << "Version could not be loaded:" << error.cause();
	}
	return true;
}

bool OneSixProfileStrategy::revertPatch(ProfilePatchPtr patch)
{
	if(!patch->isCustom())
	{
		// already not custom
		return true;
	}
	auto filename = patch->getFilename();
	if(!QFile::exists(filename))
	{
		// already gone / not custom
		return true;
	}
	// just kill the file and reload
	bool result = QFile::remove(filename);
	// FIXME: get rid of this try-catch.
	try
	{
		load();
	}
	catch (Exception &error)
	{
		qWarning() << "Version could not be loaded:" << error.cause();
	}
	return result;
}

bool OneSixProfileStrategy::installJarMods(QStringList filepaths)
{
	QString patchDir = FS::PathCombine(m_instance->instanceRoot(), "patches");
	if(!FS::ensureFolderPathExists(patchDir))
	{
		return false;
	}

	if (!FS::ensureFolderPathExists(m_instance->jarModsDir()))
	{
		return false;
	}

	for(auto filepath:filepaths)
	{
		QFileInfo sourceInfo(filepath);
		auto uuid = QUuid::createUuid();
		QString id = uuid.toString().remove('{').remove('}');
		QString target_filename = id + ".jar";
		QString target_id = "org.multimc.jarmod." + id;
		QString target_name = sourceInfo.completeBaseName() + " (jar mod)";
		QString finalPath = FS::PathCombine(m_instance->jarModsDir(), target_filename);

		QFileInfo targetInfo(finalPath);
		if(targetInfo.exists())
		{
			return false;
		}

		if (!QFile::copy(sourceInfo.absoluteFilePath(),QFileInfo(finalPath).absoluteFilePath()))
		{
			return false;
		}

		auto f = std::make_shared<VersionFile>();
		auto jarMod = std::make_shared<Library>();
		jarMod->setRawName(GradleSpecifier("org.multimc.jarmods:" + id + ":1"));
		jarMod->setFilename(target_filename);
		jarMod->setDisplayName(sourceInfo.completeBaseName());
		jarMod->setHint("local");
		f->jarMods.append(jarMod);
		f->name = target_name;
		f->uid = target_id;
		f->order = profile->getFreeOrderNumber();
		QString patchFileName = FS::PathCombine(patchDir, target_id + ".json");

		QFile file(patchFileName);
		if (!file.open(QFile::WriteOnly))
		{
			qCritical() << "Error opening" << file.fileName()
						<< "for reading:" << file.errorString();
			return false;
		}
		file.write(OneSixVersionFormat::versionFileToJson(f, true).toJson());
		file.close();

		auto patch = std::make_shared<ProfilePatch>(f, patchFileName);
		patch->setMovable(true);
		patch->setRemovable(true);
		profile->appendPatch(patch);
	}
	profile->saveCurrentOrder();
	profile->reapplyPatches();
	return true;
}

