#include "OneSixProfileStrategy.h"
#include "OneSixInstance.h"
#include "OneSixVersionFormat.h"

#include "minecraft/VersionBuildError.h"
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
	{
		auto mcJson = FS::PathCombine(m_instance->instanceRoot(), "patches" , "net.minecraft.json");
		// load up the base minecraft patch
		ProfilePatchPtr minecraftPatch;
		if(QFile::exists(mcJson))
		{
			auto file = ProfileUtils::parseJsonFile(QFileInfo(mcJson), false);
			if(file->version.isEmpty())
			{
				file->version = m_instance->intendedVersionId();
			}
			minecraftPatch = std::make_shared<ProfilePatch>(file, mcJson);
			minecraftPatch->setVanilla(false);
			minecraftPatch->setRevertible(true);
		}
		else
		{
			auto mcversion = ENV.metadataIndex()->get("net.minecraft", m_instance->intendedVersionId());
			minecraftPatch = std::make_shared<ProfilePatch>(mcversion);
			minecraftPatch->setVanilla(true);
		}
		if (!minecraftPatch)
		{
			throw VersionIncomplete("net.minecraft");
		}
		minecraftPatch->setOrder(-2);
		profile->appendPatch(minecraftPatch);
	}

	{
		auto lwjglJson = FS::PathCombine(m_instance->instanceRoot(), "patches" , "org.lwjgl.json");
		ProfilePatchPtr lwjglPatch;
		if(QFile::exists(lwjglJson))
		{
			auto file = ProfileUtils::parseJsonFile(QFileInfo(lwjglJson), false);
			lwjglPatch = std::make_shared<ProfilePatch>(file, lwjglJson);
			lwjglPatch->setVanilla(false);
			lwjglPatch->setRevertible(true);
		}
		else
		{
			auto lwjglversion = ENV.metadataIndex()->get("org.lwjgl", "2.9.1");
			lwjglPatch = std::make_shared<ProfilePatch>(lwjglversion);
			lwjglPatch->setVanilla(true);
		}
		if (!lwjglPatch)
		{
			throw VersionIncomplete("org.lwjgl");
		}
		lwjglPatch->setOrder(-1);
		profile->appendPatch(lwjglPatch);
	}
}

void OneSixProfileStrategy::loadUserPatches()
{
	// load all patches, put into map for ordering, apply in the right order
	ProfileUtils::PatchOrder userOrder;
	ProfileUtils::readOverrideOrders(FS::PathCombine(m_instance->instanceRoot(), "order.json"), userOrder);
	QDir patches(FS::PathCombine(m_instance->instanceRoot(),"patches"));
	QSet<QString> seen_extra;

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
		VersionFilePtr file = ProfileUtils::parseJsonFile(finfo, false);
		// sanity check. prevent tampering with files.
		if (file->uid != id)
		{
			file->addProblem(ProblemSeverity::Warning, QObject::tr("load id %1 does not match internal id %2").arg(id, file->uid));
			seen_extra.insert(file->uid);
		}
		auto patchEntry = std::make_shared<ProfilePatch>(file, filename);
		patchEntry->setRemovable(true);
		patchEntry->setMovable(true);
		profile->appendPatch(patchEntry);
	}
	// now load the rest by internal preference.
	using FileEntry = std::tuple<VersionFilePtr, QString>;
	QMultiMap<int, FileEntry> files;
	for (auto info : patches.entryInfoList(QStringList() << "*.json", QDir::Files))
	{
		// parse the file
		qDebug() << "Reading" << info.fileName();
		auto file = ProfileUtils::parseJsonFile(info, true);
		// ignore builtins
		if (file->uid == "net.minecraft")
			continue;
		if (file->uid == "org.lwjgl")
			continue;
		// do not load versions with broken IDs twice
		if(seen_extra.contains(file->uid))
			continue;
		// do not load what we already loaded in the first pass
		if (userOrder.contains(file->uid))
			continue;
		files.insert(file->order, std::make_tuple(file, info.filePath()));
	}
	auto appendFilePatch = [&](FileEntry tuple)
	{
		VersionFilePtr file;
		QString filename;
		std::tie(file, filename) = tuple;
		auto patchEntry = std::make_shared<ProfilePatch>(file, filename);
		patchEntry->setRemovable(true);
		patchEntry->setMovable(true);
		profile->appendPatch(patchEntry);
	};
	QSet<int> seen;
	for (auto order : files.keys())
	{
		if(seen.contains(order))
			continue;
		seen.insert(order);
		const auto &values = files.values(order);
		if(values.size() == 1)
		{
			appendFilePatch(values[0]);
			continue;
		}
		for(auto &file: values)
		{
			QStringList list;
			for(auto &file2: values)
			{
				if(file != file2)
				{
					list.append(std::get<0>(file2)->name);
				}
			}
			auto vfileptr = std::get<0>(file);
			vfileptr->addProblem(ProblemSeverity::Warning, QObject::tr("%1 has the same order as the following components:\n%2").arg(vfileptr->name, list.join(", ")));
			appendFilePatch(file);
		}
	}
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


	auto preRemoveJarMod = [&](JarmodPtr jarMod) -> bool
	{
		QString fullpath = FS::PathCombine(m_instance->jarModsDir(), jarMod->name);
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
	catch (VersionIncomplete &error)
	{
		qDebug() << "Version was incomplete:" << error.cause();
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
	try
	{
		load();
	}
	catch (VersionIncomplete &error)
	{
		qDebug() << "Version was incomplete:" << error.cause();
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
		auto jarMod = std::make_shared<Jarmod>();
		jarMod->name = target_filename;
		jarMod->originalName = sourceInfo.completeBaseName();
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

