// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Octol1ttle <l1ttleofficial@outlook.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "launch/LaunchStep.h"
#include "minecraft/MinecraftInstance.h"

class EnsureOfflineLibraries : public LaunchStep {
    Q_OBJECT

   public:
    explicit EnsureOfflineLibraries(LaunchTask* parent, MinecraftInstance* instance);
    ~EnsureOfflineLibraries() override = default;

    void executeTask() override;
    bool canAbort() const override { return false; }

   private:
    MinecraftInstance* m_instance;
};
