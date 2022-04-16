/* Copyright 2013-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Mod.h"

#include <QDir>
#include <QString>

#include <FileSystem.h>
#include <QDebug>

namespace {

ModDetails invalidDetails;

}

Mod::Mod(const QFileInfo& file)
{
    repath(file);
    m_changedDateTime = file.lastModified();
}

Mod::Mod(const QDir& mods_dir, const Packwiz::Mod& metadata)
    : m_file(mods_dir.absoluteFilePath(metadata.filename))
    // It is weird, but name is not reliable for comparing with the JAR files name
    // FIXME: Maybe use hash when implemented?
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

    m_from_metadata = true;
    m_enabled = true;
    m_changedDateTime = m_file.lastModified();
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

bool Mod::enable(bool value)
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

    if (!fromMetadata())
        repath(QFileInfo(path));

    m_enabled = value;
    return true;
}

bool Mod::destroy(QDir& index_dir)
{
    // Delete metadata
    Packwiz::deleteModIndex(index_dir, m_name);

    m_type = MOD_UNKNOWN;
    return FS::deletePath(m_file.filePath());
}

const ModDetails& Mod::details() const
{
    return m_localDetails ? *m_localDetails : invalidDetails;
}

QString Mod::name() const
{
    auto d_name = details().name;
    if (!d_name.isEmpty()) {
        return d_name;
    }
    return m_name;
}

QString Mod::version() const
{
    return details().version;
}

QString Mod::homeurl() const
{
    return details().homeurl;
}

QString Mod::description() const
{
    return details().description;
}

QStringList Mod::authors() const
{
    return details().authors;
}
