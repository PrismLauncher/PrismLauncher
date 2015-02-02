#include "logic/ftb/FTBProfileStrategy.h"
#include "logic/minecraft/VersionBuildError.h"
#include "logic/ftb/OneSixFTBInstance.h"
#include "logic/minecraft/MinecraftVersionList.h"

#include <pathutils.h>
#include <QDir>
#include <QUuid>
#include <QJsonDocument>
#include <QJsonArray>

FTBProfileStrategy::FTBProfileStrategy(OneSixFTBInstance* instance) : OneSixProfileStrategy(instance)
{
}

void FTBProfileStrategy::loadDefaultBuiltinPatches()
{
	auto mcVersion = m_instance->intendedVersionId();

	ProfilePatchPtr minecraftPatch;
	{
		auto mcJson = m_instance->versionsPath().absoluteFilePath(mcVersion + "/" + mcVersion + ".json");
		// load up the base minecraft patch
		if(QFile::exists(mcJson))
		{
			auto file = ProfileUtils::parseJsonFile(QFileInfo(mcJson), false);
			file->fileId = "net.minecraft";
			file->name = QObject::tr("Minecraft (tracked)");
			if(file->version.isEmpty())
			{
				file->version = mcVersion;
			}
			minecraftPatch = std::dynamic_pointer_cast<ProfilePatch>(file);
		}
		else
		{
			throw VersionIncomplete("net.minecraft");
		}
		minecraftPatch->setOrder(-2);
	}
	profile->appendPatch(minecraftPatch);

	ProfilePatchPtr packPatch;
	{
		auto mcJson = m_instance->minecraftRoot() + "/pack.json";
		// load up the base minecraft patch
		if(QFile::exists(mcJson))
		{
			auto file = ProfileUtils::parseJsonFile(QFileInfo(mcJson), false);

			// adapt the loaded file - the FTB patch file format is different than ours.
			file->addLibs = file->overwriteLibs;
			file->overwriteLibs.clear();
			file->shouldOverwriteLibs = false;
			file->id.clear();
			for(auto addLib: file->addLibs)
			{
				addLib->m_hint = "local";
				addLib->insertType = RawLibrary::Prepend;
			}
			file->fileId = "org.multimc.ftb.pack";
			file->name = QObject::tr("%1 (FTB pack)").arg(m_instance->name());
			if(file->version.isEmpty())
			{
				file->version = QObject::tr("Unknown");
				QFile versionFile (PathCombine(m_instance->instanceRoot(), "version"));
				if(versionFile.exists())
				{
					if(versionFile.open(QIODevice::ReadOnly))
					{
						// FIXME: just guessing the encoding/charset here.
						auto version = QString::fromUtf8(versionFile.readAll());
						file->version = version;
					}
				}
			}
			minecraftPatch = std::dynamic_pointer_cast<ProfilePatch>(file);
		}
		else
		{
			throw VersionIncomplete("org.multimc.ftb.pack");
		}
		minecraftPatch->setOrder(1);
	}
	profile->appendPatch(minecraftPatch);

}

void FTBProfileStrategy::loadUserPatches()
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


void FTBProfileStrategy::load()
{
	profile->clearPatches();

	loadDefaultBuiltinPatches();
	loadUserPatches();

	profile->finalize();
}

bool FTBProfileStrategy::saveOrder(ProfileUtils::PatchOrder order)
{
	return false;
}

bool FTBProfileStrategy::resetOrder()
{
	return false;
}

bool FTBProfileStrategy::removePatch(ProfilePatchPtr patch)
{
	return false;
}

bool FTBProfileStrategy::installJarMods(QStringList filepaths)
{
	return false;
}

