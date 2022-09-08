// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#pragma once

#include "modplatform/ModAPI.h"
#include "ui/pages/modplatform/ModPage.h"

#include "modplatform/modrinth/ModrinthAPI.h"

class ModrinthModPage : public ModPage {
    Q_OBJECT

   public:
    static ModrinthModPage* create(ModDownloadDialog* dialog, BaseInstance* instance)
    {
        return ModPage::create<ModrinthModPage>(dialog, instance);
    }

    ModrinthModPage(ModDownloadDialog* dialog, BaseInstance* instance);
    ~ModrinthModPage() override = default;

    inline auto displayName() const -> QString override { return "Modrinth"; }
    inline auto icon() const -> QIcon override { return APPLICATION->getThemedIcon("modrinth"); }
    inline auto id() const -> QString override { return "modrinth"; }
    inline auto helpPage() const -> QString override { return "Mod-platform"; }

    inline auto debugName() const -> QString override { return "Modrinth"; }
    inline auto metaEntryBase() const -> QString override { return "ModrinthPacks"; };

    auto validateVersion(ModPlatform::IndexedVersion& ver, QString mineVer, ModAPI::ModLoaderTypes loaders = ModAPI::Unspecified) const -> bool override;

    auto shouldDisplay() const -> bool override;
};
