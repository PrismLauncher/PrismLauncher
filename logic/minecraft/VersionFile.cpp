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
	return fileId == "net.minecraft";
}

bool VersionFile::hasJarMods()
{
	return !jarMods.isEmpty();
}

void VersionFile::applyTo(MinecraftProfile *profile)
{
	auto theirVersion = profile->getMinecraftVersion();
	if (!theirVersion.isNull() && !mcVersion.isNull())
	{
		if (QRegExp(mcVersion, Qt::CaseInsensitive, QRegExp::Wildcard).indexIn(theirVersion) == -1)
		{
			throw MinecraftVersionMismatch(fileId, mcVersion, theirVersion);
		}
	}
	profile->applyMinecraftVersion(id);
	profile->applyMainClass(mainClass);
	profile->applyAppletClass(appletClass);
	profile->applyMinecraftArguments(minecraftArguments);
	if (isMinecraftVersion())
	{
		profile->applyMinecraftVersionType(type);
	}
	profile->applyMinecraftAssets(assets);
	profile->applyTweakers(addTweakers);

	profile->applyJarMods(jarMods);
	profile->applyTraits(traits);

	for (auto library : libraries)
	{
		profile->applyLibrary(library);
	}
}
