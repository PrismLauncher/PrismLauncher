#include <QJsonArray>
#include <QJsonDocument>

#include <QDebug>

#include "minecraft/VersionFile.h"
#include "minecraft/RawLibrary.h"
#include "minecraft/MinecraftProfile.h"
#include "minecraft/JarMod.h"
#include "ParseUtils.h"

#include "VersionBuildError.h"
#include <Version.h>

int findLibraryByName(QList<RawLibraryPtr> haystack, const GradleSpecifier &needle)
{
	int retval = -1;
	for (int i = 0; i < haystack.size(); ++i)
	{
		if (haystack.at(i)->rawName().matchName(needle))
		{
			// only one is allowed.
			if (retval != -1)
				return -1;
			retval = i;
		}
	}
	return retval;
}

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
	if (!version->id.isNull() && !mcVersion.isNull())
	{
		if (QRegExp(mcVersion, Qt::CaseInsensitive, QRegExp::Wildcard).indexIn(version->id) ==
			-1)
		{
			throw MinecraftVersionMismatch(fileId, mcVersion, version->id);
		}
	}

	if (!id.isNull())
	{
		version->id = id;
	}
	if (!mainClass.isNull())
	{
		version->mainClass = mainClass;
	}
	if (!appletClass.isNull())
	{
		version->appletClass = appletClass;
	}
	if (!processArguments.isNull())
	{
		if (isMinecraftVersion())
		{
			version->vanillaProcessArguments = processArguments;
		}
		version->processArguments = processArguments;
	}
	if (isMinecraftVersion())
	{
		if (!type.isNull())
		{
			version->type = type;
		}
		if (!m_releaseTime.isNull())
		{
			version->m_releaseTime = m_releaseTime;
		}
		if (!m_updateTime.isNull())
		{
			version->m_updateTime = m_updateTime;
		}
	}
	if (!assets.isNull())
	{
		version->assets = assets;
	}
	if (!overwriteMinecraftArguments.isNull())
	{
		if (isMinecraftVersion())
		{
			version->vanillaMinecraftArguments = overwriteMinecraftArguments;
		}
		version->minecraftArguments = overwriteMinecraftArguments;
	}
	if (!addMinecraftArguments.isNull())
	{
		version->minecraftArguments += addMinecraftArguments;
	}
	if (shouldOverwriteTweakers)
	{
		version->tweakers = overwriteTweakers;
	}
	for (auto tweaker : addTweakers)
	{
		version->tweakers += tweaker;
	}
	version->jarMods.append(jarMods);
	version->traits.unite(traits);
	if (shouldOverwriteLibs)
	{
		QList<RawLibraryPtr> libs;
		for (auto lib : overwriteLibs)
		{
			libs.append(RawLibrary::limitedCopy(lib));
		}
		if (isMinecraftVersion())
		{
			version->vanillaLibraries = libs;
		}
		version->libraries = libs;
	}
	for (auto addedLibrary : addLibs)
	{
		// find the library by name.
		const int index = findLibraryByName(version->libraries, addedLibrary->rawName());
		// library not found? just add it.
		if (index < 0)
		{
			version->libraries.append(RawLibrary::limitedCopy(addedLibrary));
			continue;
		}
		auto existingLibrary = version->libraries.at(index);
		// if we are higher it means we should update
		if (Version(addedLibrary->version()) > Version(existingLibrary->version()))
		{
			auto library = RawLibrary::limitedCopy(addedLibrary);
			version->libraries.replace(index, library);
		}
	}
}
