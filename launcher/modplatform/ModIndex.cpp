// SPDX-License-Identifier: GPL-3.0-only
/*
*  PolyMC - Minecraft Launcher
*  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
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

namespace ModPlatform {

auto ProviderCapabilities::name(Provider p) -> const char*
{
    switch (p) {
        case Provider::MODRINTH:
            return "modrinth";
        case Provider::FLAME:
            return "curseforge";
    }
    return {};
}
auto ProviderCapabilities::readableName(Provider p) -> QString
{
    switch (p) {
        case Provider::MODRINTH:
            return "Modrinth";
        case Provider::FLAME:
            return "CurseForge";
    }
    return {};
}
auto ProviderCapabilities::hashType(Provider p) -> QStringList
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
auto ProviderCapabilities::hash(Provider p, QByteArray& data, QString type) -> QByteArray
{
    switch (p) {
        case Provider::MODRINTH: {
            // NOTE: Data is the result of reading the entire JAR file!

            // If 'type' was specified, we use that
            if (!type.isEmpty() && hashType(p).contains(type)) {
                if (type == "sha512")
                    return QCryptographicHash::hash(data, QCryptographicHash::Sha512);
                else if (type == "sha1")
                    return QCryptographicHash::hash(data, QCryptographicHash::Sha1);
            }

            return QCryptographicHash::hash(data, QCryptographicHash::Sha512);
        }
        case Provider::FLAME:
            // If 'type' was specified, we use that
            if (!type.isEmpty() && hashType(p).contains(type)) {
                if(type == "sha1")
                    return QCryptographicHash::hash(data, QCryptographicHash::Sha1);
                else if (type == "md5")
                    return QCryptographicHash::hash(data, QCryptographicHash::Md5);
            }
            
            break;
    }
    return {};
}

}  // namespace ModPlatform
