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

#include "Mod.h"

#include <QDir>
#include <QString>

#include <FileSystem.h>
#include <QDebug>

#include "Application.h"
#include "MetadataHandler.h"

namespace {

ModDetails invalidDetails;

}

Mod::Mod(const QFileInfo& file)
{
    repath(file);
    m_changedDateTime = file.lastModified();
}

Mod::Mod(const QDir& mods_dir, const Metadata::ModStruct& metadata)
    : m_file(mods_dir.absoluteFilePath(metadata.filename))
    , m_internal_id(metadata.filename)
    , m_name(metadata.name)
{
    if (m_file.isDir()) {
        m_type = MOD_FOLDER;
    } else {
        if (metadata.filename.endsWith(".zip") || metadata.filename.endsWith(".jar"))
            m_type = MOD_ZIPFILE;
        else if (metadata.filename.endsWith(".litemod"))
            m_type = MOD_LITEMOD;
        else
            m_type = MOD_SINGLEFILE;
    }

    m_enabled = true;
    m_changedDateTime = m_file.lastModified();

    m_temp_metadata = std::make_shared<Metadata::ModStruct>(std::move(metadata));
}

void Mod::repath(const QFileInfo& file)
{
    m_file = file;
    QString name_base = file.fileName();

    m_type = Mod::MOD_UNKNOWN;

    m_internal_id = name_base;

    if (m_file.isDir()) {
        m_type = MOD_FOLDER;
        m_name = name_base;
    } else if (m_file.isFile()) {
        if (name_base.endsWith(".disabled")) {
            m_enabled = false;
            name_base.chop(9);
        } else {
            m_enabled = true;
        }
        if (name_base.endsWith(".zip") || name_base.endsWith(".jar")) {
            m_type = MOD_ZIPFILE;
            name_base.chop(4);
        } else if (name_base.endsWith(".litemod")) {
            m_type = MOD_LITEMOD;
            name_base.chop(8);
        } else {
            m_type = MOD_SINGLEFILE;
        }
        m_name = name_base;
    }
}

auto Mod::enable(bool value) -> bool
{
    if (m_type == Mod::MOD_UNKNOWN || m_type == Mod::MOD_FOLDER)
        return false;

    if (m_enabled == value)
        return false;

    QString path = m_file.absoluteFilePath();
    QFile file(path);
    if (value) {
        if (!path.endsWith(".disabled"))
            return false;
        path.chop(9);

        if (!file.rename(path))
            return false;
    } else {
        path += ".disabled";

        if (!file.rename(path))
            return false;
    }

    if (status() == ModStatus::NoMetadata)
        repath(QFileInfo(path));

    m_enabled = value;
    return true;
}

void Mod::setStatus(ModStatus status)
{
    if (m_localDetails) {
        m_localDetails->status = status;
    } else {
        m_temp_status = status;
    }
}
void Mod::setMetadata(Metadata::ModStruct* metadata)
{
    if (status() == ModStatus::NoMetadata)
        setStatus(ModStatus::Installed);

    if (m_localDetails) {
        m_localDetails->metadata.reset(metadata);
    } else {
        m_temp_metadata.reset(metadata);
    }
}

auto Mod::destroy(QDir& index_dir) -> bool
{
    auto n = name();
    // FIXME: This can fail to remove the metadata if the
    // "DontUseModMetadata" setting is on, since there could
    // be a name mismatch!
    Metadata::remove(index_dir, n);

    m_type = MOD_UNKNOWN;
    return FS::deletePath(m_file.filePath());
}

auto Mod::details() const -> const ModDetails&
{
    return m_localDetails ? *m_localDetails : invalidDetails;
}

auto Mod::name() const -> QString
{
    auto d_name = details().name;
    if (!d_name.isEmpty()) {
        return d_name;
    }
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
    if (!m_localDetails)
        return m_temp_status;
    return details().status;
}

auto Mod::metadata() -> std::shared_ptr<Metadata::ModStruct>
{
    if (m_localDetails)
        return m_localDetails->metadata;
    return m_temp_metadata;
}

auto Mod::metadata() const -> const std::shared_ptr<Metadata::ModStruct>
{
    if (m_localDetails)
        return m_localDetails->metadata;
    return m_temp_metadata;
}

void Mod::finishResolvingWithDetails(std::shared_ptr<ModDetails> details)
{
    m_resolving = false;
    m_resolved = true;
    m_localDetails = details;

    if (m_localDetails && m_temp_metadata && m_temp_metadata->isValid()) {
        m_localDetails->metadata = m_temp_metadata;
        if (status() == ModStatus::NoMetadata)
            setStatus(ModStatus::Installed);
    }

    setStatus(m_temp_status);
}
