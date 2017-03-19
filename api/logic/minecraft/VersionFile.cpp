#include <QJsonArray>
#include <QJsonDocument>

#include <QDebug>

#include "minecraft/VersionFile.h"
#include "minecraft/Library.h"
#include "minecraft/MinecraftProfile.h"
#include "minecraft/JarMod.h"
#include "ParseUtils.h"

#include "VersionBuildError.h"
#include <Version.h>

bool VersionFile::isMinecraftVersion()
{
	return uid == "net.minecraft";
}

void VersionFile::applyTo(MinecraftProfile *profile)
{
	auto theirVersion = profile->getMinecraftVersion();
	if (!theirVersion.isNull() && !dependsOnMinecraftVersion.isNull())
	{
		if (QRegExp(dependsOnMinecraftVersion, Qt::CaseInsensitive, QRegExp::Wildcard).indexIn(theirVersion) == -1)
		{
			throw MinecraftVersionMismatch(uid, dependsOnMinecraftVersion, theirVersion);
		}
	}
	profile->applyMinecraftVersion(minecraftVersion);
	profile->applyMainClass(mainClass);
	profile->applyAppletClass(appletClass);
	profile->applyMinecraftArguments(minecraftArguments);
	if (isMinecraftVersion())
	{
		profile->applyMinecraftVersionType(type);
	}
	profile->applyMinecraftAssets(mojangAssetIndex);
	profile->applyTweakers(addTweakers);

	profile->applyJarMods(jarMods);
	profile->applyTraits(traits);

	for (auto library : libraries)
	{
		profile->applyLibrary(library);
	}
	profile->applyProblemSeverity(getProblemSeverity());
	auto iter = mojangDownloads.begin();
	while(iter != mojangDownloads.end())
	{
		profile->applyMojangDownload(iter.key(), iter.value());
		iter++;
	}
}
