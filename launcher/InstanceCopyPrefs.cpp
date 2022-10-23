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
