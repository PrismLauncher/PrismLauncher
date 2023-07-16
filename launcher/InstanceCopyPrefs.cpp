//
// Created by marcelohdez on 10/22/22.
//

#include "InstanceCopyPrefs.h"

bool InstanceCopyPrefs::allTrue() const
{
    return copySaves &&
        keepPlaytime &&
        copyGameOptions &&
        copyResourcePacks &&
        copyShaderPacks &&
        copyServers &&
        copyMods &&
        copyScreenshots;
}


// Returns a single RegEx string of the selected folders/files to filter out (ex: ".minecraft/saves|.minecraft/server.dat")
QString InstanceCopyPrefs::getSelectedFiltersAsRegex() const
{
    return getSelectedFiltersAsRegex({});
}
QString InstanceCopyPrefs::getSelectedFiltersAsRegex(const QStringList& additionalFilters) const
{
    QStringList filters;

    if(!copySaves)
        filters << "saves";

    if(!copyGameOptions)
        filters << "options.txt";

    if(!copyResourcePacks)
        filters << "resourcepacks" << "texturepacks";

    if(!copyShaderPacks)
        filters << "shaderpacks";

    if(!copyServers)
        filters << "servers.dat" << "servers.dat_old" << "server-resource-packs";

    if(!copyMods)
        filters << "coremods" << "mods" << "config";

    if(!copyScreenshots)
        filters << "screenshots";

    for (auto filter : additionalFilters) {
        filters << filter;
    }

    // If we have any filters to add, join them as a single regex string to return:
    if (!filters.isEmpty()) {
        const QString MC_ROOT = "[.]?minecraft/";
        // Ensure first filter starts with root, then join other filters with OR regex before root (ex: ".minecraft/saves|.minecraft/mods"):
        return MC_ROOT + filters.join("|" + MC_ROOT);
    }

    return {};
}

// ======= Getters =======
bool InstanceCopyPrefs::isCopySavesEnabled() const
{
    return copySaves;
}

bool InstanceCopyPrefs::isKeepPlaytimeEnabled() const
{
    return keepPlaytime;
}

bool InstanceCopyPrefs::isCopyGameOptionsEnabled() const
{
    return copyGameOptions;
}

bool InstanceCopyPrefs::isCopyResourcePacksEnabled() const
{
    return copyResourcePacks;
}

bool InstanceCopyPrefs::isCopyShaderPacksEnabled() const
{
    return copyShaderPacks;
}

bool InstanceCopyPrefs::isCopyServersEnabled() const
{
    return copyServers;
}

bool InstanceCopyPrefs::isCopyModsEnabled() const
{
    return copyMods;
}

bool InstanceCopyPrefs::isCopyScreenshotsEnabled() const
{
    return copyScreenshots;
}

bool InstanceCopyPrefs::isUseSymLinksEnabled() const
{
    return useSymLinks;
}

bool InstanceCopyPrefs::isUseHardLinksEnabled() const
{
    return useHardLinks;
}

bool InstanceCopyPrefs::isLinkRecursivelyEnabled() const
{
    return linkRecursively;
}

bool InstanceCopyPrefs::isDontLinkSavesEnabled() const
{
    return dontLinkSaves;
}

bool InstanceCopyPrefs::isUseCloneEnabled() const
{
    return useClone;
}

// ======= Setters =======
void InstanceCopyPrefs::enableCopySaves(bool b)
{
    copySaves = b;
}

void InstanceCopyPrefs::enableKeepPlaytime(bool b)
{
    keepPlaytime = b;
}

void InstanceCopyPrefs::enableCopyGameOptions(bool b)
{
    copyGameOptions = b;
}

void InstanceCopyPrefs::enableCopyResourcePacks(bool b)
{
    copyResourcePacks = b;
}

void InstanceCopyPrefs::enableCopyShaderPacks(bool b)
{
    copyShaderPacks = b;
}

void InstanceCopyPrefs::enableCopyServers(bool b)
{
    copyServers = b;
}

void InstanceCopyPrefs::enableCopyMods(bool b)
{
    copyMods = b;
}

void InstanceCopyPrefs::enableCopyScreenshots(bool b)
{
    copyScreenshots = b;
}

void InstanceCopyPrefs::enableUseSymLinks(bool b)
{
    useSymLinks = b;
}

void InstanceCopyPrefs::enableLinkRecursively(bool b)
{
    linkRecursively = b;
}

void InstanceCopyPrefs::enableUseHardLinks(bool b)
{
    useHardLinks = b;
}

void InstanceCopyPrefs::enableDontLinkSaves(bool b)
{
    dontLinkSaves = b;
}

void InstanceCopyPrefs::enableUseClone(bool b)
{
    useClone = b;
}