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

void VersionFile::applyTo(MinecraftProfile *version)
{
	auto theirVersion = version->getMinecraftVersion();
	if (!theirVersion.isNull() && !mcVersion.isNull())
	{
		if (QRegExp(mcVersion, Qt::CaseInsensitive, QRegExp::Wildcard).indexIn(theirVersion) == -1)
		{
			throw MinecraftVersionMismatch(fileId, mcVersion, theirVersion);
		}
	}
	version->applyMinecraftVersion(id);
	version->applyMainClass(mainClass);
	version->applyAppletClass(appletClass);
	version->applyMinecraftArguments(minecraftArguments);
	if (isMinecraftVersion())
	{
		version->applyMinecraftVersionType(type);
	}
	version->applyMinecraftAssets(assets);
	version->applyTweakers(addTweakers);

	version->applyJarMods(jarMods);
	version->applyTraits(traits);

	for (auto library : libraries)
	{
		version->applyLibrary(library);
	}
}
