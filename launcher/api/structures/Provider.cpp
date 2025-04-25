// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
 *  Copyright (c) 2023-2025 Trial97 <alexandru.tripon97@gmail.com>
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

#include "api/structures/Provider.h"

#include <QVariant>
#include "modplatform/helpers/HashUtils.h"

namespace Platform {

namespace ProviderUtils {
const char* name(Provider p)
{
    switch (p) {
        case Provider::MODRINTH:
            return "modrinth";
        case Provider::FLAME:
            return "curseforge";
    }
    return {};
}

QString readableName(Provider p)
{
    switch (p) {
        case Provider::MODRINTH:
            return "Modrinth";
        case Provider::FLAME:
            return "CurseForge";
    }
    return {};
}

QStringList hashType(Provider p)
{
    switch (p) {
        case Provider::MODRINTH:
            return { "sha512", "sha1" };
        case Provider::FLAME:
            // Try newer formats first, fall back to old format
            return { "sha1", "md5", "murmur2" };
    }
    return {};
}

QList<Hashing::Algorithm> hashTypeAlg(Provider p)
{
    switch (p) {
        case Provider::MODRINTH:
            return { Hashing::Algorithm::Sha512, Hashing::Algorithm::Sha1 };
        case Provider::FLAME:
            // Try newer formats first, fall back to old format
            return { Hashing::Algorithm::Sha1, Hashing::Algorithm::Md5, Hashing::Algorithm::Murmur2 };
    }
    return {};
}

QString getMetaURL(Provider provider, QVariant projectID)
{
    return ((provider == Provider::FLAME) ? "https://www.curseforge.com/projects/" : "https://modrinth.com/mod/") + projectID.toString();
}

}  // namespace ProviderUtils
}  // namespace Platform