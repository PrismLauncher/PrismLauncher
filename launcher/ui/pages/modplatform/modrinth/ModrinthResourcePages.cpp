// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only
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

#include "ModrinthResourcePages.h"

#include "modplatform/modrinth/ModrinthAPI.h"

#include "ui/dialogs/ResourceDownloadDialog.h"

namespace ResourceDownload {

static ResourceProviderData prepareModrinth()
{
    return {
        .displayName = Modrinth::displayName(),
        .icon = Modrinth::icon(),
        .id = Modrinth::id(),
        .metaEntryBase = Modrinth::metaEntryBase(),
        .debugName = Modrinth::debugName(),
    };
}

ShaderPackResourcePage* Modrinth::createShaderPackResourcePage(ResourceDownloadDialog* dialog, BaseInstance& instance)
{
    return new ShaderPackResourcePage(dialog, instance, prepareModrinth(), &ModrinthAPI::get());
}

DataPackResourcePage* Modrinth::createDataPackResourcePage(ResourceDownloadDialog* dialog, BaseInstance& instance)
{
    return new DataPackResourcePage(dialog, instance, prepareModrinth(), &ModrinthAPI::get());
}

ResourcePackResourcePage* Modrinth::createResourcePackResourcePage(ResourceDownloadDialog* dialog, BaseInstance& instance)
{
    return new ResourcePackResourcePage(dialog, instance, prepareModrinth(), &ModrinthAPI::get());
}

TexturePackResourcePage* Modrinth::createTexturePackResourcePage(ResourceDownloadDialog* dialog, BaseInstance& instance)
{
    return new TexturePackResourcePage(dialog, instance, prepareModrinth(), &ModrinthAPI::get());
}

ModPage* Modrinth::createModPage(ResourceDownloadDialog* dialog, BaseInstance& instance)
{
    return new ModPage(dialog, instance, prepareModrinth(), &ModrinthAPI::get(),
                       ModFilterWidget::create(&static_cast<MinecraftInstance&>(instance), true));
}
}  // namespace ResourceDownload
