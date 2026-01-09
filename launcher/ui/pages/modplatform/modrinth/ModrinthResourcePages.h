// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only AND Apache-2.0
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (c) 2023 Trial97 <alexandru.tripon97@gmail.com>
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

#include "modplatform/ResourceAPI.h"

#include "ui/pages/modplatform/DataPackPage.h"
#include "ui/pages/modplatform/ModPage.h"
#include "ui/pages/modplatform/ResourcePackPage.h"
#include "ui/pages/modplatform/ShaderPackPage.h"
#include "ui/pages/modplatform/TexturePackPage.h"

namespace ResourceDownload {

namespace Modrinth {
static inline QString displayName()
{
    return "Modrinth";
}
static inline QIcon icon()
{
    return QIcon::fromTheme("modrinth");
}
static inline QString id()
{
    return "modrinth";
}
static inline QString debugName()
{
    return "Modrinth";
}
static inline QString metaEntryBase()
{
    return "ModrinthPacks";
}
}  // namespace Modrinth

class ModrinthModPage : public ModPage {
    Q_OBJECT

   public:
    static ModrinthModPage* create(ModDownloadDialog* dialog, BaseInstance& instance)
    {
        return ModPage::create<ModrinthModPage>(dialog, instance);
    }

    ModrinthModPage(ModDownloadDialog* dialog, BaseInstance& instance);
    ~ModrinthModPage() override = default;

    bool shouldDisplay() const override;

    inline auto displayName() const -> QString override { return Modrinth::displayName(); }
    inline auto icon() const -> QIcon override { return Modrinth::icon(); }
    inline auto id() const -> QString override { return Modrinth::id(); }
    inline auto debugName() const -> QString override { return Modrinth::debugName(); }
    inline auto metaEntryBase() const -> QString override { return Modrinth::metaEntryBase(); }

    inline auto helpPage() const -> QString override { return "Mod-platform"; }

    std::unique_ptr<ModFilterWidget> createFilterWidget() override;

   protected:
    virtual void prepareProviderCategories() override;
    Task::Ptr m_categoriesTask;
};

class ModrinthResourcePackPage : public ResourcePackResourcePage {
    Q_OBJECT

   public:
    static ModrinthResourcePackPage* create(ResourcePackDownloadDialog* dialog, BaseInstance& instance)
    {
        return ResourcePackResourcePage::create<ModrinthResourcePackPage>(dialog, instance);
    }

    ModrinthResourcePackPage(ResourcePackDownloadDialog* dialog, BaseInstance& instance);
    ~ModrinthResourcePackPage() override = default;

    bool shouldDisplay() const override;

    inline auto displayName() const -> QString override { return Modrinth::displayName(); }
    inline auto icon() const -> QIcon override { return Modrinth::icon(); }
    inline auto id() const -> QString override { return Modrinth::id(); }
    inline auto debugName() const -> QString override { return Modrinth::debugName(); }
    inline auto metaEntryBase() const -> QString override { return Modrinth::metaEntryBase(); }

    inline auto helpPage() const -> QString override { return ""; }
};

class ModrinthTexturePackPage : public TexturePackResourcePage {
    Q_OBJECT

   public:
    static ModrinthTexturePackPage* create(TexturePackDownloadDialog* dialog, BaseInstance& instance)
    {
        return TexturePackResourcePage::create<ModrinthTexturePackPage>(dialog, instance);
    }

    ModrinthTexturePackPage(TexturePackDownloadDialog* dialog, BaseInstance& instance);
    ~ModrinthTexturePackPage() override = default;

    bool shouldDisplay() const override;

    inline auto displayName() const -> QString override { return Modrinth::displayName(); }
    inline auto icon() const -> QIcon override { return Modrinth::icon(); }
    inline auto id() const -> QString override { return Modrinth::id(); }
    inline auto debugName() const -> QString override { return Modrinth::debugName(); }
    inline auto metaEntryBase() const -> QString override { return Modrinth::metaEntryBase(); }

    inline auto helpPage() const -> QString override { return ""; }
};

class ModrinthShaderPackPage : public ShaderPackResourcePage {
    Q_OBJECT

   public:
    static ModrinthShaderPackPage* create(ShaderPackDownloadDialog* dialog, BaseInstance& instance)
    {
        return ShaderPackResourcePage::create<ModrinthShaderPackPage>(dialog, instance);
    }

    ModrinthShaderPackPage(ShaderPackDownloadDialog* dialog, BaseInstance& instance);
    ~ModrinthShaderPackPage() override = default;

    bool shouldDisplay() const override;

    inline auto displayName() const -> QString override { return Modrinth::displayName(); }
    inline auto icon() const -> QIcon override { return Modrinth::icon(); }
    inline auto id() const -> QString override { return Modrinth::id(); }
    inline auto debugName() const -> QString override { return Modrinth::debugName(); }
    inline auto metaEntryBase() const -> QString override { return Modrinth::metaEntryBase(); }

    inline auto helpPage() const -> QString override { return ""; }
};

class ModrinthDataPackPage : public DataPackResourcePage {
    Q_OBJECT

   public:
    static ModrinthDataPackPage* create(DataPackDownloadDialog* dialog, BaseInstance& instance)
    {
        return DataPackResourcePage::create<ModrinthDataPackPage>(dialog, instance);
    }

    ModrinthDataPackPage(DataPackDownloadDialog* dialog, BaseInstance& instance);
    ~ModrinthDataPackPage() override = default;

    bool shouldDisplay() const override;

    inline auto displayName() const -> QString override { return Modrinth::displayName(); }
    inline auto icon() const -> QIcon override { return Modrinth::icon(); }
    inline auto id() const -> QString override { return Modrinth::id(); }
    inline auto debugName() const -> QString override { return Modrinth::debugName(); }
    inline auto metaEntryBase() const -> QString override { return Modrinth::metaEntryBase(); }

    inline auto helpPage() const -> QString override { return ""; }
};

}  // namespace ResourceDownload
