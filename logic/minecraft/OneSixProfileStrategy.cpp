#include "logic/minecraft/OneSixProfileStrategy.h"
#include "logic/minecraft/VersionBuildError.h"
#include "logic/OneSixInstance.h"
#include "logic/minecraft/MinecraftVersionList.h"

#include "MultiMC.h"

#include <pathutils.h>
#include <QDir>
#include <QUuid>
#include <QJsonDocument>
#include <QJsonArray>

OneSixProfileStrategy::OneSixProfileStrategy(OneSixInstance* instance)
{
	m_instance = instance;
}

void OneSixProfileStrategy::upgradeDeprecatedFiles()
{
	auto versionJsonPath = PathCombine(m_instance->instanceRoot(), "version.json");
	auto customJsonPath = PathCombine(m_instance->instanceRoot(), "custom.json");
	auto mcJson = PathCombine(m_instance->instanceRoot(), "patches" , "net.minecraft.json");

	// convert old crap.
	if(QFile::exists(customJsonPath))
	{
		if(!ensureFilePathExists(mcJson))
		{
			// WHAT DO???
		}
		if(!QFile::rename(customJsonPath, mcJson))
		{
			// WHAT DO???
		}
		if(QFile::exists(versionJsonPath))
		{
			if(!QFile::remove(versionJsonPath))
			{
				// WHAT DO???
			}
		}
	}
	else if(QFile::exists(versionJsonPath))
	{
		if(!ensureFilePathExists(mcJson))
		{
			// WHAT DO???
		}
		if(!QFile::rename(versionJsonPath, mcJson))
		{
			// WHAT DO???
		}
	}
}


void OneSixProfileStrategy::loadDefaultBuiltinPatches()
{
	auto mcJson = PathCombine(m_instance->instanceRoot(), "patches" , "net.minecraft.json");
	// load up the base minecraft patch
	ProfilePatchPtr minecraftPatch;
	if(QFile::exists(mcJson))
	{
		auto file = ProfileUtils::parseJsonFile(QFileInfo(mcJson), false);
		file->fileId = "net.minecraft";
		file->name = "Minecraft";
		if(file->version.isEmpty())
		{
			file->version = m_instance->intendedVersionId();
		}
		minecraftPatch = std::dynamic_pointer_cast<ProfilePatch>(file);
	}
	else
	{
		auto minecraftList = MMC->minecraftlist();
		auto mcversion = minecraftList->findVersion(m_instance->intendedVersionId());
		minecraftPatch = std::dynamic_pointer_cast<ProfilePatch>(mcversion);
	}
	if (!minecraftPatch)
	{
		throw VersionIncomplete("net.minecraft");
	}
	minecraftPatch->setOrder(-2);
	profile->appendPatch(minecraftPatch);


	// TODO: this is obviously fake.
	QResource LWJGL(":/versions/LWJGL/2.9.1.json");
	auto lwjgl = ProfileUtils::parseJsonFile(LWJGL.absoluteFilePath(), false);
	auto lwjglPatch = std::dynamic_pointer_cast<ProfilePatch>(lwjgl);
	if (!lwjglPatch)
	{
		throw VersionIncomplete("org.lwjgl");
	}
	lwjglPatch->setOrder(-1);
	lwjgl->setVanilla(true);
	profile->appendPatch(lwjglPatch);
}

void OneSixProfileStrategy::loadUserPatches()
{
	// load all patches, put into map for ordering, apply in the right order
	ProfileUtils::PatchOrder userOrder;
	ProfileUtils::readOverrideOrders(PathCombine(m_instance->instanceRoot(), "order.json"), userOrder);
	QDir patches(PathCombine(m_instance->instanceRoot(),"patches"));

	// first, load things by sort order.
	for (auto id : userOrder)
	{
		// ignore builtins
		if (id == "net.minecraft")
			continue;
		if (id == "org.lwjgl")
			continue;
		// parse the file
		QString filename = patches.absoluteFilePath(id + ".json");
		QFileInfo finfo(filename);
		if(!finfo.exists())
		{
			QLOG_INFO() << "Patch file " << filename << " was deleted by external means...";
			continue;
		}
		QLOG_INFO() << "Reading" << filename << "by user order";
		auto file = ProfileUtils::parseJsonFile(finfo, false);
		// sanity check. prevent tampering with files.
		if (file->fileId != id)
		{
			throw VersionBuildError(
				QObject::tr("load id %1 does not match internal id %2").arg(id, file->fileId));
		}
		profile->appendPatch(file);
	}
	// now load the rest by internal preference.
	QMap<int, QPair<QString, VersionFilePtr>> files;
	for (auto info : patches.entryInfoList(QStringList() << "*.json", QDir::Files))
	{
		// parse the file
		QLOG_INFO() << "Reading" << info.fileName();
		auto file = ProfileUtils::parseJsonFile(info, true);
		// ignore builtins
		if (file->fileId == "net.minecraft")
			continue;
		if (file->fileId == "org.lwjgl")
			continue;
		// do not load what we already loaded in the first pass
		if (userOrder.contains(file->fileId))
			continue;
		if (files.contains(file->order))
		{
			// FIXME: do not throw?
			throw VersionBuildError(QObject::tr("%1 has the same order as %2")
										.arg(file->fileId, files[file->order].second->fileId));
		}
		files.insert(file->order, qMakePair(info.fileName(), file));
	}
	for (auto order : files.keys())
	{
		auto &filePair = files[order];
		profile->appendPatch(filePair.second);
	}
}


void OneSixProfileStrategy::load()
{
	profile->clearPatches();

	upgradeDeprecatedFiles();
	loadDefaultBuiltinPatches();
	loadUserPatches();

	profile->finalize();
}

bool OneSixProfileStrategy::saveOrder(ProfileUtils::PatchOrder order)
{
	return ProfileUtils::writeOverrideOrders(PathCombine(m_instance->instanceRoot(), "order.json"), order);
}

bool OneSixProfileStrategy::resetOrder()
{
	return QDir(m_instance->instanceRoot()).remove("order.json");
}

bool OneSixProfileStrategy::removePatch(ProfilePatchPtr patch)
{
	bool ok = true;
	// first, remove the patch file. this ensures it's not used anymore
	auto fileName = patch->getPatchFilename();


	auto preRemoveJarMod = [&](JarmodPtr jarMod) -> bool
	{
		QString fullpath = PathCombine(m_instance->jarModsDir(), jarMod->name);
		QFileInfo finfo (fullpath);
		if(finfo.exists())
		{
			return QFile::remove(fullpath);
		}
		return true;
	};

	for(auto &jarmod: patch->getJarMods())
	{
		ok &= preRemoveJarMod(jarmod);
	}
	return ok;
}

bool OneSixProfileStrategy::installJarMods(QStringList filepaths)
{
	QString patchDir = PathCombine(m_instance->instanceRoot(), "patches");
	if(!ensureFolderPathExists(patchDir))
	{
		return false;
	}

	if (!ensureFolderPathExists(m_instance->jarModsDir()))
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
		QString finalPath = PathCombine(m_instance->jarModsDir(), target_filename);

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
		auto jarMod = std::make_shared<Jarmod>();
		jarMod->name = target_filename;
		f->jarMods.append(jarMod);
		f->name = target_name;
		f->fileId = target_id;
		f->order = profile->getFreeOrderNumber();
		QString patchFileName = PathCombine(patchDir, target_id + ".json");
		f->filename = patchFileName;

		QFile file(patchFileName);
		if (!file.open(QFile::WriteOnly))
		{
			QLOG_ERROR() << "Error opening" << file.fileName()
						<< "for reading:" << file.errorString();
			return false;
		}
		file.write(f->toJson(true).toJson());
		file.close();
		profile->appendPatch(f);
	}
	profile->saveCurrentOrder();
	profile->reapply();
	return true;
}

