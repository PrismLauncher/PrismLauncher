#include "minecraft/OneSixProfileStrategy.h"
#include "minecraft/VersionBuildError.h"
#include "minecraft/OneSixInstance.h"
#include "minecraft/MinecraftVersionList.h"
#include "Env.h"

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
		if(!ensureFilePathExists(mcJson))
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
		file->fileId = "net.minecraft";
		file->version = file->id;
		file->name = "Minecraft";
		auto data = file->toJson(false).toJson();
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
	auto mcJson = PathCombine(m_instance->instanceRoot(), "patches" , "net.minecraft.json");
	// load up the base minecraft patch
	ProfilePatchPtr minecraftPatch;
	if(QFile::exists(mcJson))
	{
		auto file = ProfileUtils::parseJsonFile(QFileInfo(mcJson), false);
		if(file->version.isEmpty())
		{
			file->version = m_instance->intendedVersionId();
		}
		file->setVanilla(false);
		minecraftPatch = std::dynamic_pointer_cast<ProfilePatch>(file);
	}
	else
	{
		auto mcversion = ENV.getVersion("net.minecraft", m_instance->intendedVersionId());
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
			qDebug() << "Patch file " << filename << " was deleted by external means...";
			continue;
		}
		qDebug() << "Reading" << filename << "by user order";
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
		qDebug() << "Reading" << info.fileName();
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
	if(fileName.size())
	{
		QFile patchFile(fileName);
		if(patchFile.exists() && !patchFile.remove())
		{
			qCritical() << "File" << fileName << "could not be removed because:" << patchFile.errorString();
			return false;
		}
	}


	auto preRemoveJarMod = [&](JarmodPtr jarMod) -> bool
	{
		QString fullpath = PathCombine(m_instance->jarModsDir(), jarMod->name);
		QFileInfo finfo (fullpath);
		if(finfo.exists())
		{
			QFile jarModFile(fullpath);
			if(!jarModFile.remove())
			{
				qCritical() << "File" << fullpath << "could not be removed because:" << jarModFile.errorString();
				return false;
			}
			return true;
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
			qCritical() << "Error opening" << file.fileName()
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

