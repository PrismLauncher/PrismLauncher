// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 Mai Lapyst <67418776+Mai-Lapyst@users.noreply.github.com>
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
 */
#include "CatPackExtension.h"

#include <QJsonArray>

#include "Application.h"
#include "plugin/Plugin.h"
#include "ui/themes/CatPack.h"
#include "ui/themes/ThemeManager.h"

bool CatPackContribution::loadConfig(const Plugin& plugin, const QJsonValue& json)
{
    QDir dir = QDir(plugin.relativePath(json.toString()));
    if (!dir.exists() || !dir.isReadable())
        return false;

    m_manifest = QFileInfo(dir.absoluteFilePath("catpack.json"));

    return m_manifest.exists() && m_manifest.isFile() && m_manifest.isReadable();
}

void CatPackContribution::onPluginEnable()
{
    m_id = APPLICATION->themeManager()->addCatPack(std::unique_ptr<CatPack>(new JsonCatPack(m_manifest)));
    qInfo(pluginLogC) << "Added cat pack" << m_id;
}

void CatPackContribution::onPluginDisable()
{
    if (m_id.isEmpty())
        return;

    APPLICATION->themeManager()->removeCatPack(m_id);
    m_id = "";  // reset to empty
}
