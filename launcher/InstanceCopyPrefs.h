//
// Created by marcelohdez on 10/22/22.
//

#pragma once

#include <QStringList>

struct InstanceCopyPrefs {
   public:
    [[nodiscard]] bool allTrue() const;
    [[nodiscard]] QString getSelectedFiltersAsRegex() const;
    // Getters
    [[nodiscard]] bool isCopySavesEnabled() const;
    [[nodiscard]] bool isKeepPlaytimeEnabled() const;
    [[nodiscard]] bool isCopyGameOptionsEnabled() const;
    [[nodiscard]] bool isCopyResourcePacksEnabled() const;
    [[nodiscard]] bool isCopyShaderPacksEnabled() const;
    [[nodiscard]] bool isCopyServersEnabled() const;
    [[nodiscard]] bool isCopyModsEnabled() const;
    [[nodiscard]] bool isCopyScreenshotsEnabled() const;
    // Setters
    void enableCopySaves(bool b);
    void enableKeepPlaytime(bool b);
    void enableCopyGameOptions(bool b);
    void enableCopyResourcePacks(bool b);
    void enableCopyShaderPacks(bool b);
    void enableCopyServers(bool b);
    void enableCopyMods(bool b);
    void enableCopyScreenshots(bool b);

   protected: // data
    bool copySaves = true;
    bool keepPlaytime = true;
    bool copyGameOptions = true;
    bool copyResourcePacks = true;
    bool copyShaderPacks = true;
    bool copyServers = true;
    bool copyMods = true;
    bool copyScreenshots = true;
};
