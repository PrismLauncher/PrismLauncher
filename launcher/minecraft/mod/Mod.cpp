// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
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

#include "Mod.h"

#include <QDir>
#include <QRegularExpression>
#include <QString>
#include <algorithm>

#include "MTPixmapCache.h"
#include "Resource.h"
#include "Version.h"
#include "minecraft/mod/ModDetails.h"
#include "minecraft/mod/tasks/LocalModParseTask.h"
#include "modplatform/ModIndex.h"

namespace {

int compareVersionLists(const QStringList& leftVersions, const QStringList& rightVersions)
{
    const qsizetype commonSize = std::min(leftVersions.size(), rightVersions.size());

    for (qsizetype i = 0; i < commonSize; i++) {
        const auto leftVersion = Version(leftVersions.at(i).trimmed());
        const auto rightVersion = Version(rightVersions.at(i).trimmed());

        if (leftVersion > rightVersion)
            return 1;

        if (leftVersion < rightVersion)
            return -1;
    }

    if (leftVersions.size() > rightVersions.size())
        return 1;

    if (leftVersions.size() < rightVersions.size())
        return -1;

    return 0;
}

}  // namespace

Mod::Mod(const QFileInfo& file) : Resource(file)
{
    m_enabled = (file.suffix() != "disabled");
}

void Mod::setDetails(const ModDetails& details)
{
    m_localDetails = details;
}

int Mod::compare(const Resource& other, SortType type) const
{
    const auto* castOther = dynamic_cast<const Mod*>(&other);
    if (!castOther) {
        return Resource::compare(other, type);
    }

    switch (type) {
        default:
        case SortType::Enabled:
        case SortType::Name:
        case SortType::Date:
        case SortType::Size:
            return Resource::compare(other, type);
        case SortType::Version: {
            auto thisVer = Version(version());
            auto otherVer = Version(castOther->version());
            if (thisVer > otherVer) {
                return 1;
            }
            if (thisVer < otherVer) {
                return -1;
            }
            break;
        }
        case SortType::Side: {
            auto compareResult = QString::compare(side(), castOther->side(), Qt::CaseInsensitive);
            if (compareResult != 0) {
                return compareResult;
            }
            break;
        }
        case SortType::McVersions: {
            auto compareResult = compareVersionLists(mcVersions(), castOther->mcVersions());
            if (compareResult != 0) {
                return compareResult;
            }
            break;
        }
        case SortType::Loaders: {
            auto compareResult = QString::compare(loaders(), castOther->loaders(), Qt::CaseInsensitive);
            if (compareResult != 0) {
                return compareResult;
            }
            break;
        }
        case SortType::ReleaseType: {
            auto compareResult = QString::compare(releaseType(), castOther->releaseType(), Qt::CaseInsensitive);
            if (compareResult != 0) {
                return compareResult;
            }
            break;
        }
        case SortType::RequiredBy: {
            if (requiredByCount() > castOther->requiredByCount()) {
                return 1;
            }
            if (requiredByCount() < castOther->requiredByCount()) {
                return -1;
            }
            break;
        }
        case SortType::Requires: {
            if (requiresCount() > castOther->requiresCount()) {
                return 1;
            }
            if (requiresCount() < castOther->requiresCount()) {
                return -1;
            }
            break;
        }
    }
    return 0;
}

bool Mod::applyFilter(const QRegularExpression& filter) const
{
    if (filter.match(description()).hasMatch()) {
        return true;
    }

    for (auto& author : authors()) {
        if (filter.match(author).hasMatch()) {
            return true;
        }
    }

    return Resource::applyFilter(filter);
}

auto Mod::details() const -> const ModDetails&
{
    return m_localDetails;
}

auto Mod::name() const -> QString
{
    auto dName = details().name;
    if (!dName.isEmpty()) {
        return dName;
    }

    return Resource::name();
}

auto Mod::modId() const -> QString
{
    auto dModId = details().mod_id;
    if (!dModId.isEmpty()) {
        return dModId;
    }

    return Resource::name();
}

auto Mod::version() const -> QString
{
    return details().version;
}

auto Mod::homepage() const -> QString
{
    QString metaUrl = Resource::homepage();

    if (metaUrl.isEmpty()) {
        return details().homeurl;
    }
    return metaUrl;
}

auto Mod::loaders() const -> QString
{
    if (metadata()) {
        QStringList loaders;
        auto modLoaders = metadata()->loaders;
        for (auto loader : ModPlatform::modLoaderTypesToList(modLoaders)) {
            loaders << getModLoaderAsString(loader);
        }
        return loaders.join(", ");
    }

    return {};
}

auto Mod::side() const -> QString
{
    if (metadata()) {
        return metadata()->side.toString();
    }

    return ModPlatform::SideType(ModPlatform::SideType::UniversalSide).toString();
}

auto Mod::mcVersions() const -> QStringList
{
    if (metadata()) {
        return metadata()->mcVersions;
    }

    return {};
}

auto Mod::mcVersionsString() const -> QString
{
    return mcVersions().join(", ");
}

auto Mod::releaseType() const -> QString
{
    if (metadata()) {
        return metadata()->releaseType.toString();
    }

    return ModPlatform::IndexedVersionType(ModPlatform::IndexedVersionType::Unknown).toString();
}

auto Mod::description() const -> QString
{
    return details().description;
}

auto Mod::authors() const -> QStringList
{
    return details().authors;
}

void Mod::finishResolvingWithDetails(ModDetails&& details)
{
    m_isResolving = false;
    m_isResolved = true;

    m_localDetails = std::move(details);
    if (!iconPath().isEmpty()) {
        m_packImageCacheKey.wasReadAttempt = false;
    }
}

auto Mod::licenses() const -> const QList<ModLicense>&
{
    return details().licenses;
}

auto Mod::issueTracker() const -> QString
{
    return details().issue_tracker;
}

QPixmap Mod::setIcon(const QImage& newImage) const
{
    QMutexLocker locker(&m_dataLock);

    Q_ASSERT(!newImage.isNull());

    if (m_packImageCacheKey.key.isValid()) {
        PixmapCache::remove(m_packImageCacheKey.key);
    }

    // scale the image to avoid flooding the pixmapcache
    auto pixmap =
        QPixmap::fromImage(newImage.scaled({ 64, 64 }, Qt::AspectRatioMode::KeepAspectRatioByExpanding, Qt::SmoothTransformation));

    m_packImageCacheKey.key = PixmapCache::insert(pixmap);
    m_packImageCacheKey.wasEverUsed = true;
    m_packImageCacheKey.wasReadAttempt = true;
    return pixmap;
}

QPixmap Mod::icon(QSize size, Qt::AspectRatioMode mode) const
{
    auto pixmapTransform = [&size, &mode](QPixmap pixmap) {
        if (size.isNull()) {
            return pixmap;
        }
        return pixmap.scaled(size, mode, Qt::SmoothTransformation);
    };

    QPixmap cachedImage;
    if (PixmapCache::find(m_packImageCacheKey.key, &cachedImage)) {
        return pixmapTransform(cachedImage);
    }

    // No valid image we can get
    if ((!m_packImageCacheKey.wasEverUsed && m_packImageCacheKey.wasReadAttempt) || iconPath().isEmpty()) {
        return {};
    }

    if (m_packImageCacheKey.wasEverUsed) {
        qDebug() << "Mod" << name() << "Had it's icon evicted from the cache. reloading...";
        PixmapCache::markCacheMissByEviciton();
    }
    // Image got evicted from the cache or an attempt to load it has not been made. load it and retry.
    m_packImageCacheKey.wasReadAttempt = true;
    if (ModUtils::loadIconFile(*this, &cachedImage)) {
        return pixmapTransform(cachedImage);
    }
    // Image failed to load
    return {};
}

bool Mod::valid() const
{
    return !m_localDetails.mod_id.isEmpty();
}

QStringList Mod::dependencies() const
{
    return details().dependencies;
}

int Mod::requiredByCount() const
{
    return m_requiredByCount;
}
int Mod::requiresCount() const
{
    return m_requiresCount;
}
void Mod::setRequiredByCount(int value)
{
    m_requiredByCount = value;
}
void Mod::setRequiresCount(int value)
{
    m_requiresCount = value;
}
