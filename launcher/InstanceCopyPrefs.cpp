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
        copyMods;
}

// Returns a single RegEx string of the selected folders/files to filter out (ex: ".minecraft/saves|.minecraft/server.dat")
QString InstanceCopyPrefs::getSelectedFiltersAsRegex() const
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

    // If we have any filters to add, join them as a single regex string to return:
    if (!filters.isEmpty()) {
        const QString MC_ROOT = "[.]?minecraft/";
        // Ensure first filter starts with root, then join other filters with OR regex before root (ex: ".minecraft/saves|.minecraft/mods"):
        return MC_ROOT + filters.join("|" + MC_ROOT);
    }

    return {};
}
