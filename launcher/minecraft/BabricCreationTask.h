// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 */

#pragma once

#include "InstanceCreationTask.h"
#include "minecraft/PackProfile.h"

class BabricCreationTask final : public InstanceCreationTask {
    Q_OBJECT
   public:
    BabricCreationTask() = default;

    /**
     * Writes the four Babric patch files from embedded string literals into
     * the instance's patches/ directory and registers all five component
     * versions in the PackProfile.
     *
     * Safe to call on an already-configured Babric instance (re-install).
     * Does NOT call resolve() — the caller is responsible for that.
     *
     * Returns false on any I/O error.
     */
    static bool installPatches(PackProfile* profile);

    /**
     * Removes all Babric-related components from the profile and deletes
     * their local patch files.  net.minecraft is intentionally kept so the
     * user can later install a different loader or change the MC version.
     */
    static void removePatches(PackProfile* profile);

    std::unique_ptr<MinecraftInstance> createInstance() override;
};
