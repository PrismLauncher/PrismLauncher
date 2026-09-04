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

ShaderPackResourcePage* createShaderPackResourcePage(ResourceDownloadDialog* dialog, MinecraftInstance& instance);
DataPackResourcePage* createDataPackResourcePage(ResourceDownloadDialog* dialog, MinecraftInstance& instance);
ResourcePackResourcePage* createResourcePackResourcePage(ResourceDownloadDialog* dialog, MinecraftInstance& instance);
TexturePackResourcePage* createTexturePackResourcePage(ResourceDownloadDialog* dialog, MinecraftInstance& instance);
ModPage* createModPage(ResourceDownloadDialog* dialog, MinecraftInstance& instance);
}  // namespace Modrinth

}  // namespace ResourceDownload
