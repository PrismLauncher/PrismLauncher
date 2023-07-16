//
// Created by marcelohdez on 10/22/22.
//

#pragma once

#include <QStringList>

struct InstanceCopyPrefs {
   public:
    [[nodiscard]] bool allTrue() const;
    [[nodiscard]] QString getSelectedFiltersAsRegex() const;
    [[nodiscard]] QString getSelectedFiltersAsRegex(const QStringList& additionalFilters) const;
    // Getters
    [[nodiscard]] bool isCopySavesEnabled() const;
    [[nodiscard]] bool isKeepPlaytimeEnabled() const;
    [[nodiscard]] bool isCopyGameOptionsEnabled() const;
    [[nodiscard]] bool isCopyResourcePacksEnabled() const;
    [[nodiscard]] bool isCopyShaderPacksEnabled() const;
    [[nodiscard]] bool isCopyServersEnabled() const;
    [[nodiscard]] bool isCopyModsEnabled() const;
    [[nodiscard]] bool isCopyScreenshotsEnabled() const;
    [[nodiscard]] bool isUseSymLinksEnabled() const;
    [[nodiscard]] bool isLinkRecursivelyEnabled() const;
    [[nodiscard]] bool isUseHardLinksEnabled() const;
    [[nodiscard]] bool isDontLinkSavesEnabled() const;
    [[nodiscard]] bool isUseCloneEnabled() const;
    // Setters
    void enableCopySaves(bool b);
    void enableKeepPlaytime(bool b);
    void enableCopyGameOptions(bool b);
    void enableCopyResourcePacks(bool b);
    void enableCopyShaderPacks(bool b);
    void enableCopyServers(bool b);
    void enableCopyMods(bool b);
    void enableCopyScreenshots(bool b);
    void enableUseSymLinks(bool b);
    void enableLinkRecursively(bool b);
    void enableUseHardLinks(bool b);
    void enableDontLinkSaves(bool b);
    void enableUseClone(bool b);

   protected: // data
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
