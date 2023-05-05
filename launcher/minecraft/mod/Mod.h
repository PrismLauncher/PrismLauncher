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
#include <QList>
#include <QImage>
#include <QMutex>
#include <QPixmap>
#include <QPixmapCache>

#include <optional>

#include "Resource.h"
#include "ModDetails.h"

class Mod : public Resource
{
    Q_OBJECT
public:
    using Ptr = shared_qobject_ptr<Mod>;
    using WeakPtr = QPointer<Mod>;

    Mod() = default;
    Mod(const QFileInfo &file);
    Mod(const QDir& mods_dir, const Metadata::ModStruct& metadata);
    Mod(QString file_path) : Mod(QFileInfo(file_path)) {}

    auto details()     const -> const ModDetails&;
    auto name()        const -> QString override;
    auto version()     const -> QString;
    auto homeurl()     const -> QString;
    auto description() const -> QString;
    auto authors()     const -> QStringList;
    auto status()      const -> ModStatus;
    auto provider()    const -> std::optional<QString>;
    auto licenses()     const -> const QList<ModLicense>&;
    auto issueTracker() const -> QString;

    /** Get the intneral path to the mod's icon file*/
    QString iconPath() const { return m_local_details.icon_file; };
    /** Gets the icon of the mod, converted to a QPixmap for drawing, and scaled to size. */
    [[nodiscard]] QPixmap icon(QSize size, Qt::AspectRatioMode mode = Qt::AspectRatioMode::IgnoreAspectRatio) const;
    /** Thread-safe. */
    void setIcon(QImage new_image) const;

    auto metadata() -> std::shared_ptr<Metadata::ModStruct>;
    auto metadata() const -> const std::shared_ptr<Metadata::ModStruct>;

    void setStatus(ModStatus status);
    void setMetadata(std::shared_ptr<Metadata::ModStruct>&& metadata);
    void setMetadata(const Metadata::ModStruct& metadata) { setMetadata(std::make_shared<Metadata::ModStruct>(metadata)); }
    void setDetails(const ModDetails& details);

    bool valid() const override;

    [[nodiscard]] auto compare(Resource const& other, SortType type) const -> std::pair<int, bool> override;
    [[nodiscard]] bool applyFilter(QRegularExpression filter) const override;

    // Delete all the files of this mod
    auto destroy(QDir& index_dir, bool preserve_metadata = false) -> bool;

    void finishResolvingWithDetails(ModDetails&& details);

protected:
    ModDetails m_local_details;

    mutable QMutex m_data_lock;

    struct {
        QPixmapCache::Key key;
        bool was_ever_used = false;
        bool was_read_attempt = false;
    } mutable m_pack_image_cache_key;
    
};
