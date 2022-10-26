//
// Created by marcelohdez on 10/22/22.
//

#ifndef LAUNCHER_INSTANCECOPYPREFS_H
#define LAUNCHER_INSTANCECOPYPREFS_H

#include <QStringList>

struct InstanceCopyPrefs {
    bool copySaves = true;
    bool keepPlaytime = true;
    bool copyGameOptions = true;
    bool copyResourcePacks = true;
    bool copyShaderPacks = true;
    bool copyServers = true;
    bool copyMods = true;

    [[nodiscard]] bool allTrue() const;
    [[nodiscard]] QString getSelectedFiltersAsRegex() const;
};

#endif  // LAUNCHER_INSTANCECOPYPREFS_H
