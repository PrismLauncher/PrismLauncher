// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
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

#include "modplatform/ModIndex.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QIODevice>

namespace ModPlatform {
const char* ProviderCapabilities::name(ResourceProvider p)
{
    switch (p) {
        case ResourceProvider::MODRINTH:
            return "modrinth";
        case ResourceProvider::FLAME:
            return "curseforge";
    }
    return {};
}

QString ProviderCapabilities::readableName(ResourceProvider p)
{
    switch (p) {
        case ResourceProvider::MODRINTH:
            return "Modrinth";
        case ResourceProvider::FLAME:
            return "CurseForge";
    }
    return {};
}

QStringList ProviderCapabilities::hashType(ResourceProvider p)
{
    switch (p) {
        case ResourceProvider::MODRINTH:
            return { "sha512", "sha1" };
        case ResourceProvider::FLAME:
            // Try newer formats first, fall back to old format
            return { "sha1", "md5", "murmur2" };
    }
    return {};
}

QString getMetaURL(ResourceProvider provider, QVariant projectID)
{
    return ((provider == ModPlatform::ResourceProvider::FLAME) ? "https://www.curseforge.com/projects/" : "https://modrinth.com/mod/") +
           projectID.toString();
}
}  // namespace ModPlatform
