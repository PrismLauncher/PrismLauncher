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
#include <qpixmap.h>

#include <QDir>
#include <QRegularExpression>
#include <QString>

#include "MTPixmapCache.h"
#include "MetadataHandler.h"
#include "Resource.h"
#include "Version.h"
#include "minecraft/mod/ModDetails.h"
#include "minecraft/mod/tasks/LocalModParseTask.h"
#include "modplatform/ModIndex.h"
#include "Application.h"
#include "meta/Index.h"
#include "ui/widgets/VersionSelectWidget.h"
#include "ui/widgets/VersionListView.h"
#include <sstream> 

Mod::Mod(const QFileInfo& file) : Resource(file), m_local_details()
{
    m_enabled = (file.suffix() != "disabled");
}

void Mod::setDetails(const ModDetails& details)
{
    m_local_details = details;
}

int Mod::compare(const Resource& other, SortType type) const
{
    auto cast_other = dynamic_cast<Mod const*>(&other);
    if (!cast_other)
        return Resource::compare(other, type);

    switch (type) {
        default:
        case SortType::ENABLED:
        case SortType::NAME:
        case SortType::DATE:
        case SortType::SIZE:
            return Resource::compare(other, type);
        case SortType::VERSION: {
            auto this_ver = Version(version());
            auto other_ver = Version(cast_other->version());
            if (this_ver > other_ver)
                return 1;
            if (this_ver < other_ver)
                return -1;
            break;
        }
        case SortType::SIDE: {
            auto compare_result = QString::compare(side(), cast_other->side(), Qt::CaseInsensitive);
            if (compare_result != 0)
                return compare_result;
            break;
        }
        case SortType::MC_VERSIONS: {
            auto compare_result = QString::compare(mcVersions(), cast_other->mcVersions(), Qt::CaseInsensitive);
            if (compare_result != 0)
                return compare_result;
            break;
        }
        case SortType::LOADERS: {
            auto compare_result = QString::compare(loaders(), cast_other->loaders(), Qt::CaseInsensitive);
            if (compare_result != 0)
                return compare_result;
            break;
        }
        case SortType::RELEASE_TYPE: {
            auto compare_result = QString::compare(releaseType(), cast_other->releaseType(), Qt::CaseInsensitive);
            if (compare_result != 0)
                return compare_result;
            break;
        }
    }
    return 0;
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

auto Mod::details() const -> const ModDetails&
{
    return m_local_details;
}

auto Mod::name() const -> QString
{
    auto d_name = details().name;
    if (!d_name.isEmpty())
        return d_name;

    return Resource::name();
}

auto Mod::mod_id() const -> QString
{
    auto d_mod_id = details().mod_id;
    if (!d_mod_id.isEmpty())
        return d_mod_id;

    return Resource::name();
}

auto Mod::version() const -> QString
{
    return details().version;
}

auto Mod::homepage() const -> QString
{
    QString metaUrl = Resource::homepage();

    if (metaUrl.isEmpty())
        return details().homeurl;
    else
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
    if (metadata())
        return ModPlatform::SideUtils::toString(metadata()->side);

    return ModPlatform::SideUtils::toString(ModPlatform::Side::UniversalSide);
}

auto Mod::mcVersions() const -> QString
{
    if (metadata())
        // incase toggelable feature
        if (1) {
            return groupMcVersions();
        } else {
            return metadata()->mcVersions.join(", ");
        }

    return {};
}

auto Mod::groupMcVersions() const -> QString
{
    static QStringList s_realeasedMcVersions = []() {
         // Gets all of minecrafts vanilla versions
        auto vlist = APPLICATION->metadataIndex()->get("net.minecraft");
        VersionSelectWidget* versionList = new VersionSelectWidget(nullptr);
        versionList->initialize(vlist.get());
        //Filters to only main released versions (like 1.19.2, 1.8.9)
        versionList->setFilter(BaseVersionList::TypeRole, Filters::regexp(QRegularExpression("(release)")));
        auto model = versionList->view()->model();
        QAbstractItemModel* proxyModel = model;
        // Versions get ordered latest to oldest
        QStringList filteredVersions;
        // extracts each version number from model
        for (int row = 0; row < proxyModel->rowCount(); ++row) {
            QModelIndex index = proxyModel->index(row, 0);
            QVariant data = proxyModel->data(index, BaseVersionList::VersionRole);
            filteredVersions << data.toString();
        }
        return filteredVersions;
    }();
    
    QStringList filteredVersions = s_realeasedMcVersions;

    QStringList *groupedVersions = new QStringList();

    if (metadata()) {
        std::vector<QString> myvector; 
        for (QString i : metadata()->mcVersions) {
            if (i.size() >= 10) {
                // snapshot get filtered out, can add separte control fro them here
            } else {
                myvector.push_back(i);
            }
        }
        std::sort(myvector.begin(), myvector.end(), versionsSort);
        QStringList ungroupedVersions(myvector.begin(), myvector.end());
        //QStringList ungroupedVersions = metadata()->mcVersions;
        
        QString first = ungroupedVersions[0];
        int count = 0;
        int ungroupedSize = ungroupedVersions.size();
        bool grouping = false;
        for (int i = filteredVersions.size() - 1; i >= 0; --i) {
            if (grouping) {
                // traversing the ungrouped versions and checking if they match the order of releases
                if (! filteredVersions[i].compare(ungroupedVersions[count])) {
                    count += 1;
                    if (count >= ungroupedSize) {
                        break;
                    }
                } 
                // next version doesnt match, break grouping, append and update next grouping
                else {
                    grouping = false;
                    // check if grouping occured
                    if (! first.compare(ungroupedVersions[count - 1])) {
                        (*groupedVersions).append(first);
                    } else {
                        QString groupBuff = first.append('-').append(ungroupedVersions[count - 1]);
                        (*groupedVersions).append(groupBuff);
                    }
                    // set up next grouping
                    first = ungroupedVersions[count];
                }
            } else {
                // traverses all oldest to newest versions until modded version starts
                if (! filteredVersions.at(i).compare(first)) {
                    grouping = true;
                    count += 1;
                    if (count >= ungroupedSize) {
                        break;
                    }
                }
            }
        }
        // case for last version included in mod
        if (grouping) {
            if (! first.compare(ungroupedVersions[count - 1])) {
                (*groupedVersions).append(first);
            } else {
                QString groupBuff = first.append('-').append(ungroupedVersions[count - 1]);
                (*groupedVersions).append(groupBuff);
            }
        }
        return (*groupedVersions).join(", ");
    }

    return {};
       

}

// return false if a > b
bool Mod::versionsSort(QString a, QString b)
{
    std::string stringA = a.toStdString();
    std::string stringB = b.toStdString();
    std::stringstream streamA(stringA);
    std::stringstream streamB(stringB);
    std::string bufA;
    std::string bufB;

    while (getline(streamA, bufA, '.') && getline(streamB, bufB, '.')) {
        int intA = atoi(bufA.c_str());
        int intB = atoi(bufB.c_str());
        if (intA == intB) {
            continue;
        } else if (intA > intB) {
            return false;
        } else {
            return true;
        }
    }
    // one is an 1.x instead of 1.x.y so the small length string is the lower version
    if (stringA.size() > stringB.size()) {
        return false;
    } else {
        return true;
    }
}

auto Mod::releaseType() const -> QString
{
    if (metadata())
        return metadata()->releaseType.toString();

    return ModPlatform::IndexedVersionType().toString();
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
    m_is_resolving = false;
    m_is_resolved = true;

    m_local_details = std::move(details);
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

QPixmap Mod::setIcon(QImage new_image) const
{
    QMutexLocker locker(&m_data_lock);

    Q_ASSERT(!new_image.isNull());

    if (m_packImageCacheKey.key.isValid())
        PixmapCache::remove(m_packImageCacheKey.key);

    // scale the image to avoid flooding the pixmapcache
    auto pixmap =
        QPixmap::fromImage(new_image.scaled({ 64, 64 }, Qt::AspectRatioMode::KeepAspectRatioByExpanding, Qt::SmoothTransformation));

    m_packImageCacheKey.key = PixmapCache::insert(pixmap);
    m_packImageCacheKey.wasEverUsed = true;
    m_packImageCacheKey.wasReadAttempt = true;
    return pixmap;
}

QPixmap Mod::icon(QSize size, Qt::AspectRatioMode mode) const
{
    auto pixmap_transform = [&size, &mode](QPixmap pixmap) {
        if (size.isNull())
            return pixmap;
        return pixmap.scaled(size, mode, Qt::SmoothTransformation);
    };

    QPixmap cached_image;
    if (PixmapCache::find(m_packImageCacheKey.key, &cached_image)) {
        return pixmap_transform(cached_image);
    }

    // No valid image we can get
    if ((!m_packImageCacheKey.wasEverUsed && m_packImageCacheKey.wasReadAttempt) || iconPath().isEmpty())
        return {};

    if (m_packImageCacheKey.wasEverUsed) {
        qDebug() << "Mod" << name() << "Had it's icon evicted from the cache. reloading...";
        PixmapCache::markCacheMissByEviciton();
    }
    // Image got evicted from the cache or an attempt to load it has not been made. load it and retry.
    m_packImageCacheKey.wasReadAttempt = true;
    if (ModUtils::loadIconFile(*this, &cached_image)) {
        return pixmap_transform(cached_image);
    }
    // Image failed to load
    return {};
}

bool Mod::valid() const
{
    return !m_local_details.mod_id.isEmpty();
}
