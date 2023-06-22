// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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
 */
#include "ExportModsToStringTask.h"
#include "modplatform/ModIndex.h"

namespace ExportToString {
QString ExportModsToStringTask(QList<Mod*> mods, Formats format, OptionalData extraData)
{
    switch (format) {
        case HTML: {
            QStringList lines;
            for (auto mod : mods) {
                auto meta = mod->metadata();
                auto modName = mod->name();
                if (extraData & Url) {
                    auto url = mod->homeurl();
                    if (meta != nullptr) {
                        url = (meta->provider == ModPlatform::ResourceProvider::FLAME ? "https://www.curseforge.com/minecraft/mc-mods/"
                                                                                      : "https://modrinth.com/mod/") +
                              meta->project_id.toString();
                    }
                    if (!url.isEmpty())
                        modName = QString("<a href=\"%1\">%2</a>").arg(url, modName);
                }
                auto line = modName;
                if (extraData & Version) {
                    auto ver = mod->version();
                    if (ver.isEmpty() && meta != nullptr)
                        ver = meta->version().toString();
                    if (!ver.isEmpty())
                        line += QString("[%1]").arg(ver);
                }
                if (extraData & Authors && !mod->authors().isEmpty())
                    line += " by " + mod->authors().join(", ");
                lines.append(QString("<ul>%1</ul>").arg(line));
            }
            return QString("<html><body>\n\t%1\n</body></html>").arg(lines.join("\n\t"));
        }
        case MARKDOWN: {
            QStringList lines;
            for (auto mod : mods) {
                auto meta = mod->metadata();
                auto modName = mod->name();
                if (extraData & Url) {
                    auto url = mod->homeurl();
                    if (meta != nullptr) {
                        url = (meta->provider == ModPlatform::ResourceProvider::FLAME ? "https://www.curseforge.com/minecraft/mc-mods/"
                                                                                      : "https://modrinth.com/mod/") +
                              meta->project_id.toString();
                    }
                    if (!url.isEmpty())
                        modName = QString("[%1](%2)").arg(modName, url);
                }
                auto line = modName;
                if (extraData & Version) {
                    auto ver = mod->version();
                    if (ver.isEmpty() && meta != nullptr)
                        ver = meta->version().toString();
                    if (!ver.isEmpty())
                        line += QString("[%1]").arg(ver);
                }
                if (extraData & Authors && !mod->authors().isEmpty())
                    line += " by " + mod->authors().join(", ");
                lines << line;
            }
            return lines.join("\n");
        }
        default: {
            return QString("unknown format:%1").arg(format);
        }
    }
}

QString ExportModsToStringTask(QList<Mod*> mods, QString lineTemplate)
{
    QStringList lines;
    for (auto mod : mods) {
        auto meta = mod->metadata();
        auto modName = mod->name();

        auto url = mod->homeurl();
        if (meta != nullptr) {
            url = (meta->provider == ModPlatform::ResourceProvider::FLAME ? "https://www.curseforge.com/minecraft/mc-mods/"
                                                                          : "https://modrinth.com/mod/") +
                  meta->project_id.toString();
        }
        auto ver = mod->version();
        if (ver.isEmpty() && meta != nullptr)
            ver = meta->version().toString();
        auto authors = mod->authors().join(", ");
        lines << QString(lineTemplate)
                     .replace("{name}", modName)
                     .replace("{url}", url)
                     .replace("{version}", ver)
                     .replace("{authors}", authors);
    }
    return lines.join("\n");
}
}  // namespace ExportToString