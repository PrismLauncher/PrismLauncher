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

#pragma once

#include <QDateTime>
#include <QFileInfo>
#include <QImage>
#include <QList>
#include <QMutex>
#include <QPixmap>
#include <QPixmapCache>

#include "ModDetails.h"
#include "Resource.h"

class Mod : public Resource {
   public:
    explicit Mod(const QFileInfo& file);
    explicit Mod(const QString& filePath) : Mod(QFileInfo(filePath)) {}

    auto details() const -> const ModDetails&;
    auto name() const -> QString override;
    auto modId() const -> QString;
    auto version() const -> QString;
    auto homepage() const -> QString override;
    auto description() const -> QString;
    auto authors() const -> QStringList;
    auto licenses() const -> const QList<ModLicense>&;
    auto issueTracker() const -> QString;
    auto side() const -> QString;
    auto loaders() const -> QString;
    auto mcVersions() const -> QStringList;
    auto mcVersionsString() const -> QString;
    auto releaseType() const -> QString;
    QStringList dependencies() const;

    int requiredByCount() const;
    int requiresCount() const;

    void setRequiredByCount(int value);
    void setRequiresCount(int value);

    /** Get the intneral path to the mod's icon file*/
    QString iconPath() const { return m_localDetails.icon_file; }
    /** Gets the icon of the mod, converted to a QPixmap for drawing, and scaled to size. */
    QPixmap icon(QSize size, Qt::AspectRatioMode mode = Qt::AspectRatioMode::IgnoreAspectRatio) const;
    /** Thread-safe. */
    QPixmap setIcon(const QImage& newImage) const;

    void setDetails(const ModDetails& details);

    bool valid() const override;

    [[nodiscard]] int compare(const Resource& other, SortType type) const override;
    [[nodiscard]] bool applyFilter(const QRegularExpression& filter) const override;

    // Delete all the files of this mod
    auto destroy(QDir& indexDir, bool preserveMetadata = false, bool attemptTrash = true) -> bool;
    // Delete the metadata only
    void destroyMetadata(QDir& indexDir);

    void finishResolvingWithDetails(ModDetails&& details);

   protected:
    ModDetails m_localDetails;

    mutable QMutex m_dataLock;

    struct {
        QPixmapCache::Key key;
        bool wasEverUsed = false;
        bool wasReadAttempt = false;
    } mutable m_packImageCacheKey;

    int m_requiredByCount = 0;
    int m_requiresCount = 0;
};
