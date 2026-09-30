// SPDX-License-Identifier: GPL-3.0-only
/*
 *  MaterialMC - Minecraft Launcher
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

#include <QDateTime>
#include <QJsonValue>
#include <QUrl>

#include "Application.h"
#include "InstanceList.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

#include "ApiParams.h"

namespace api {

/** Looks up the instance named by `params[key]` or throws INSTANCE_NOT_FOUND. */
inline MinecraftInstance* requireInstance(const QJsonObject& params, const QString& key = QStringLiteral("id"))
{
    const auto id = params::requireNonEmpty(params, key, 512);
    auto* instance = APPLICATION->instances()->getInstanceById(id);
    if (!instance) {
        throw ApiError::instanceNotFound(id);
    }
    return instance;
}

/**
 * Component lists are loaded lazily; load them from disk (never from the network) before reading versions.
 * Same approach as MinecraftInstance::getStatusbarDescription(). Returns the profile, or nullptr.
 */
inline PackProfile* loadedPackProfile(MinecraftInstance* instance)
{
    auto* profile = instance->getPackProfile();
    if (profile && profile->getComponentVersion("net.minecraft").isEmpty()) {
        if (auto res = profile->reload(Net::Mode::Offline); !res) {
            qWarning() << "Failed to load the components of" << instance->id() << ":" << res.error();
        }
    }
    return profile;
}

/** Milliseconds since epoch, or null for "unknown". */
inline QJsonValue timestamp(qint64 msecs)
{
    return msecs > 0 ? QJsonValue(static_cast<double>(msecs)) : QJsonValue();
}

inline QJsonValue timestamp(const QDateTime& dt)
{
    return dt.isValid() ? QJsonValue(static_cast<double>(dt.toMSecsSinceEpoch())) : QJsonValue();
}

/** URL of a host-generated resource, e.g. hostResourceUrl({"_icon", key}) -> materialmc://app/_icon/<key>. */
inline QString hostResourceUrl(const QStringList& segments)
{
    QStringList encoded;
    encoded.reserve(segments.size());
    for (const auto& s : segments) {
        encoded << QString::fromUtf8(QUrl::toPercentEncoding(s));
    }
    return QStringLiteral("materialmc://app/") + encoded.join('/');
}

inline QJsonObject ok()
{
    return { { "ok", true } };
}

}  // namespace api
