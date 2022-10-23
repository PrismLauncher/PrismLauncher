//
// Created by marcelohdez on 10/22/22.
//

#include "InstanceCopyPrefs.h"

InstanceCopyPrefs::InstanceCopyPrefs(bool setAll)
    : copySaves(setAll),
    keepPlaytime(setAll),
    copyGameOptions(setAll),
    copyResourcePacks(setAll),
    copyShaderPacks(setAll),
    copyServers(setAll),
    copyMods(setAll)
{}
