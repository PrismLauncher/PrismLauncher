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
#include <QDebug>
#include <QIODevice>

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

auto ProviderCapabilities::hash(Provider p, QIODevice* device, QString type) -> QString
{
    QCryptographicHash::Algorithm algo = QCryptographicHash::Sha1;
    switch (p) {
        case Provider::MODRINTH: {
            algo = (type == "sha1") ? QCryptographicHash::Sha1 : QCryptographicHash::Sha512;
            break;
        }
        case Provider::FLAME:
            algo = (type == "sha1") ? QCryptographicHash::Sha1 : QCryptographicHash::Md5;
            break;
    }

    QCryptographicHash hash(algo);
    if(!hash.addData(device))
        qCritical() << "Failed to read JAR to create hash!";

    Q_ASSERT(hash.result().length() == hash.hashLength(algo));
    return { hash.result().toHex() };
}

}  // namespace ModPlatform
