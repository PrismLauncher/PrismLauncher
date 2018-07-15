#include <QJsonArray>
#include <QJsonDocument>

#include <QDebug>

#include "minecraft/VersionFile.h"
#include "minecraft/Library.h"
#include "minecraft/ComponentList.h"
#include "ParseUtils.h"

#include <Version.h>

static bool isMinecraftVersion(const QString &uid)
{
    return uid == "net.minecraft";
}

void VersionFile::applyTo(LaunchProfile *profile)
{
    // Only real Minecraft can set those. Don't let anything override them.
    if (isMinecraftVersion(uid))
    {
        profile->applyMinecraftVersion(minecraftVersion);
        profile->applyMinecraftVersionType(type);
        // HACK: ignore assets from other version files than Minecraft
        // workaround for stupid assets issue caused by amazon:
        // https://www.theregister.co.uk/2017/02/28/aws_is_awol_as_s3_goes_haywire/
        profile->applyMinecraftAssets(mojangAssetIndex);
    }

    profile->applyMainJar(mainJar);
    profile->applyMainClass(mainClass);
    profile->applyAppletClass(appletClass);
    profile->applyMinecraftArguments(minecraftArguments);
    profile->applyTweakers(addTweakers);
    profile->applyJarMods(jarMods);
    profile->applyMods(mods);
    profile->applyTraits(traits);

    for (auto library : libraries)
    {
        profile->applyLibrary(library);
    }
    profile->applyProblemSeverity(getProblemSeverity());
}

/*
    auto theirVersion = profile->getMinecraftVersion();
    if (!theirVersion.isNull() && !dependsOnMinecraftVersion.isNull())
    {
        if (QRegExp(dependsOnMinecraftVersion, Qt::CaseInsensitive, QRegExp::Wildcard).indexIn(theirVersion) == -1)
        {
            throw MinecraftVersionMismatch(uid, dependsOnMinecraftVersion, theirVersion);
        }
    }
*/