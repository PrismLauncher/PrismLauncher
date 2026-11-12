// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2026 Prism Launcher Contributors
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

#include <QIcon>

#include "ui/pages/modplatform/ModPage.h"

class BaseInstance;
class ResourceFolderModel;

namespace ResourceDownload {

class ResourceDownloadDialog;

namespace Github {

static inline QString displayName()
{
    return QStringLiteral("GitHub");
}
static inline QIcon icon()
{
    return QIcon::fromTheme(QStringLiteral("github"));
}
static inline QString id()
{
    return QStringLiteral("github");
}
static inline QString debugName()
{
    return QStringLiteral("GitHub");
}
static inline QString metaEntryBase()
{
    return QStringLiteral("GithubMods");
}

ModPage* createModPage(ResourceDownloadDialog* dialog, BaseInstance& instance);

}  // namespace Github

}  // namespace ResourceDownload
