//
// Created by marcelohdez on 10/22/22.
//

#pragma once

#include <QStringList>

struct InstanceCopyPrefs {
    bool allTrue() const;
    QString getSelectedFiltersAsRegex() const;
    QString getSelectedFiltersAsRegex(const QStringList& additionalFilters) const;

    bool copySaves = true;
    bool keepPlaytime = true;
    bool copyGameOptions = true;
    bool copyResourcePacks = true;
    bool copyShaderPacks = true;
    bool copyServers = true;
    bool copyMods = true;
    bool copyScreenshots = true;
    bool useSymLinks = false;
    bool linkRecursively = false;
    bool useHardLinks = false;
    bool dontLinkSaves = false;
    bool useClone = false;
};
