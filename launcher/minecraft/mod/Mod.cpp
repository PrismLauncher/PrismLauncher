// SPDX-License-Identifier: GPL-3.0-only
/*
*  PolyMC - Minecraft Launcher
*  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
*  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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

#include <QDebug>
#include <QDir>
#include <QString>
#include <QRegularExpression>

#include "MTPixmapCache.h"
#include "MetadataHandler.h"
#include "Version.h"
#include "minecraft/mod/ModDetails.h"
#include "minecraft/mod/tasks/LocalModParseTask.h"

static ModPlatform::ProviderCapabilities ProviderCaps;

Mod::Mod(const QFileInfo& file) : Resource(file), m_local_details()
{
    m_enabled = (file.suffix() != "disabled");
}

Mod::Mod(const QDir& mods_dir, const Metadata::ModStruct& metadata)
    : Mod(mods_dir.absoluteFilePath(metadata.filename))
{
    m_name = metadata.name;
    m_local_details.metadata = std::make_shared<Metadata::ModStruct>(std::move(metadata));
}

void Mod::setStatus(ModStatus status)
{
    m_local_details.status = status;
}
void Mod::setMetadata(std::shared_ptr<Metadata::ModStruct>&& metadata)
{
    if (status() == ModStatus::NoMetadata)
        setStatus(ModStatus::Installed);

    m_local_details.metadata = metadata;
}

void Mod::setDetails(const ModDetails& details) {
    m_local_details = details;
}

std::pair<int, bool> Mod::compare(const Resource& other, SortType type) const
{
    auto cast_other = dynamic_cast<Mod const*>(&other);
    if (!cast_other)
        return Resource::compare(other, type);

    switch (type) {
        default:
        case SortType::ENABLED:
        case SortType::NAME:
        case SortType::DATE: {
            auto res = Resource::compare(other, type);
            if (res.first != 0)
                return res;
        }
        case SortType::VERSION: {
            auto this_ver = Version(version());
            auto other_ver = Version(cast_other->version());
            if (this_ver > other_ver)
                return { 1, type == SortType::VERSION };
            if (this_ver < other_ver)
                return { -1, type == SortType::VERSION };
        }
        case SortType::PROVIDER: {
            auto compare_result = QString::compare(provider().value_or("Unknown"), cast_other->provider().value_or("Unknown"), Qt::CaseInsensitive);
            if (compare_result != 0)
                return { compare_result, type == SortType::PROVIDER };
        }
    }
    return { 0, false };
}

bool Mod::applyFilter(QRegularExpression filter) const
{
    if (filter.match(description()).hasMatch())
        return true;

    for (auto& author : authors()) {
        if (filter.match(author).hasMatch()) {
            return true;
        }
    }

    return Resource::applyFilter(filter);
}

auto Mod::destroy(QDir& index_dir, bool preserve_metadata) -> bool
{
    if (!preserve_metadata) {
        qDebug() << QString("Destroying metadata for '%1' on purpose").arg(name());

        if (metadata()) {
            Metadata::remove(index_dir, metadata()->slug);
        } else {
            auto n = name();
            Metadata::remove(index_dir, n);
        }
    }

    return Resource::destroy();
}

auto Mod::details() const -> const ModDetails&
{
    return m_local_details;
}

auto Mod::name() const -> QString
{
    auto d_name = details().name;
    if (!d_name.isEmpty())
        return d_name;

    if (metadata())
        return metadata()->name;

    return m_name;
}

auto Mod::version() const -> QString
{
    return details().version;
}

auto Mod::homeurl() const -> QString
{
    return details().homeurl;
}

auto Mod::description() const -> QString
{
    return details().description;
}

auto Mod::authors() const -> QStringList
{
    return details().authors;
}

auto Mod::status() const -> ModStatus
{
    return details().status;
}

auto Mod::metadata() -> std::shared_ptr<Metadata::ModStruct>
{
    return m_local_details.metadata;
}

auto Mod::metadata() const -> const std::shared_ptr<Metadata::ModStruct>
{
    return m_local_details.metadata;
}

void Mod::finishResolvingWithDetails(ModDetails&& details)
{
    m_is_resolving = false;
    m_is_resolved = true;

    std::shared_ptr<Metadata::ModStruct> metadata = details.metadata;
    if (details.status == ModStatus::Unknown)
        details.status = m_local_details.status;

    m_local_details = std::move(details);
    if (metadata)
        setMetadata(std::move(metadata));
    if (!iconPath().isEmpty()) {
        m_pack_image_cache_key.was_read_attempt = false;
    }
}

auto Mod::provider() const -> std::optional<QString>
{
    if (metadata())
        return ProviderCaps.readableName(metadata()->provider);
    return {};
}

auto Mod::licenses() const -> const QList<ModLicense>&
{
    return details().licenses;
}

 auto Mod::issueTracker() const -> QString
{
    return details().issue_tracker;
}

void Mod::setIcon(QImage new_image) const
{
    QMutexLocker locker(&m_data_lock);

    Q_ASSERT(!new_image.isNull());

    if (m_pack_image_cache_key.key.isValid())
        PixmapCache::remove(m_pack_image_cache_key.key);

    // scale the image to avoid flooding the pixmapcache
    auto pixmap = QPixmap::fromImage(new_image.scaled({64, 64}, Qt::AspectRatioMode::KeepAspectRatioByExpanding));

    m_pack_image_cache_key.key = PixmapCache::insert(pixmap);
    m_pack_image_cache_key.was_ever_used = true;
    m_pack_image_cache_key.was_read_attempt = true;
}

QPixmap Mod::icon(QSize size, Qt::AspectRatioMode mode) const
{
    QPixmap cached_image;
    if (PixmapCache::find(m_pack_image_cache_key.key, &cached_image)) {
        if (size.isNull())
            return cached_image;
        return cached_image.scaled(size, mode);
    }

    // No valid image we can get
    if ((!m_pack_image_cache_key.was_ever_used && m_pack_image_cache_key.was_read_attempt) || iconPath().isEmpty())
        return {};

    if (m_pack_image_cache_key.was_ever_used) {
        qDebug() << "Mod" << name() << "Had it's icon evicted form the cache. reloading...";
        PixmapCache::markCacheMissByEviciton();
    }
    // Image got evicted from the cache or an attempt to load it has not been made. load it and retry.
    m_pack_image_cache_key.was_read_attempt = true;
    ModUtils::loadIconFile(*this);
    return icon(size);
}

bool Mod::valid() const
{
    return !m_local_details.mod_id.isEmpty();
}
