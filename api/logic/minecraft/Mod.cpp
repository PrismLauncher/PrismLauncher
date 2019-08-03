/* Copyright 2013-2019 MultiMC Contributors
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

#include <QDir>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <quazip.h>
#include <quazipfile.h>

#include "Mod.h"
#include "settings/INIFile.h"
#include <FileSystem.h>
#include <QDebug>

namespace {
// NEW format
// https://github.com/MinecraftForge/FML/wiki/FML-mod-information-file/6f62b37cea040daf350dc253eae6326dd9c822c3

// OLD format:
// https://github.com/MinecraftForge/FML/wiki/FML-mod-information-file/5bf6a2d05145ec79387acc0d45c958642fb049fc
ModDetails ReadMCModInfo(QByteArray contents)
{
    auto getInfoFromArray = [&](QJsonArray arr)->ModDetails
    {
        ModDetails details;
        if (!arr.at(0).isObject()) {
            return details;
        }
        auto firstObj = arr.at(0).toObject();
        details.mod_id = firstObj.value("modid").toString();
        auto name = firstObj.value("name").toString();
        // NOTE: ignore stupid example mods copies where the author didn't even bother to change the name
        if(name != "Example Mod") {
            details.name = name;
        }
        details.version = firstObj.value("version").toString();
        details.updateurl = firstObj.value("updateUrl").toString();
        auto homeurl = firstObj.value("url").toString().trimmed();
        if(!homeurl.isEmpty())
        {
            // fix up url.
            if (!homeurl.startsWith("http://") && !homeurl.startsWith("https://") && !homeurl.startsWith("ftp://"))
            {
                homeurl.prepend("http://");
            }
        }
        details.homeurl = homeurl;
        details.description = firstObj.value("description").toString();
        QJsonArray authors = firstObj.value("authorList").toArray();
        if (authors.size() == 0) {
            // FIXME: what is the format of this? is there any?
            authors = firstObj.value("authors").toArray();
        }

        for (auto author: authors)
        {
            details.authors.append(author.toString());
        }
        details.credits = firstObj.value("credits").toString();
        details.valid = true;
        return details;
    };
    QJsonParseError jsonError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(contents, &jsonError);
    // this is the very old format that had just the array
    if (jsonDoc.isArray())
    {
        return getInfoFromArray(jsonDoc.array());
    }
    else if (jsonDoc.isObject())
    {
        auto val = jsonDoc.object().value("modinfoversion");
        if(val.isUndefined()) {
            val = jsonDoc.object().value("modListVersion");
        }
        int version = val.toDouble();
        if (version != 2)
        {
            qCritical() << "BAD stuff happened to mod json:";
            qCritical() << contents;
            return ModDetails();
        }
        auto arrVal = jsonDoc.object().value("modlist");
        if(arrVal.isUndefined()) {
            arrVal = jsonDoc.object().value("modList");
        }
        if (arrVal.isArray())
        {
            return getInfoFromArray(arrVal.toArray());
        }
    }
    return ModDetails();
}

// https://fabricmc.net/wiki/documentation:fabric_mod_json
ModDetails ReadFabricModInfo(QByteArray contents)
{
    QJsonParseError jsonError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(contents, &jsonError);
    auto object = jsonDoc.object();
    auto schemaVersion = object.contains("schemaVersion") ? object.value("schemaVersion").toInt(0) : 0;

    ModDetails details;

    details.mod_id = object.value("id").toString();
    details.version = object.value("version").toString();

    details.name = object.contains("name") ? object.value("name").toString() : details.mod_id;
    details.description = object.value("description").toString();

    if (schemaVersion >= 1)
    {
        QJsonArray authors = object.value("authors").toArray();
        for (auto author: authors)
        {
            if(author.isObject()) {
                details.authors.append(author.toObject().value("name").toString());
            }
            else {
                details.authors.append(author.toString());
            }
        }

        if (object.contains("contact"))
        {
            QJsonObject contact = object.value("contact").toObject();

            if (contact.contains("homepage"))
            {
                details.homeurl = contact.value("homepage").toString();
            }
        }
    }
    details.valid = !details.name.isEmpty();
    return details;
}

ModDetails ReadForgeInfo(QByteArray contents)
{
    ModDetails details;
    // Read the data
    details.name = "Minecraft Forge";
    details.mod_id = "Forge";
    details.homeurl = "http://www.minecraftforge.net/forum/";
    details.valid = true;
    INIFile ini;
    if (!ini.loadFile(contents))
        return details;

    QString major = ini.get("forge.major.number", "0").toString();
    QString minor = ini.get("forge.minor.number", "0").toString();
    QString revision = ini.get("forge.revision.number", "0").toString();
    QString build = ini.get("forge.build.number", "0").toString();

    details.version = major + "." + minor + "." + revision + "." + build;
    return details;
}

ModDetails ReadLiteModInfo(QByteArray contents)
{
    ModDetails details;
    QJsonParseError jsonError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(contents, &jsonError);
    auto object = jsonDoc.object();
    if (object.contains("name"))
    {
        details.mod_id = details.name = object.value("name").toString();
    }
    if (object.contains("version"))
    {
        details.version = object.value("version").toString("");
    }
    else
    {
        details.version = object.value("revision").toString("");
    }
    details.mcversion = object.value("mcversion").toString();
    auto author = object.value("author").toString();
    if(!author.isEmpty()) {
        details.authors.append(author);
    }
    details.description = object.value("description").toString();
    details.homeurl = object.value("url").toString();
    return details;
}

ModDetails invalidDetails;

}


Mod::Mod(const QFileInfo &file)
{
    repath(file);
    m_changedDateTime = file.lastModified();
}

void Mod::repath(const QFileInfo &file)
{
    m_file = file;
    QString name_base = file.fileName();

    m_type = Mod::MOD_UNKNOWN;

    if (m_file.isDir())
    {
        m_type = MOD_FOLDER;
        m_name = name_base;
        m_mmc_id = name_base;
    }
    else if (m_file.isFile())
    {
        if (name_base.endsWith(".disabled"))
        {
            m_enabled = false;
            name_base.chop(9);
        }
        else
        {
            m_enabled = true;
        }
        m_mmc_id = name_base;
        if (name_base.endsWith(".zip") || name_base.endsWith(".jar"))
        {
            m_type = MOD_ZIPFILE;
            name_base.chop(4);
        }
        else if (name_base.endsWith(".litemod"))
        {
            m_type = MOD_LITEMOD;
            name_base.chop(8);
        }
        else
        {
            m_type = MOD_SINGLEFILE;
        }
        m_name = name_base;
    }

    if (m_type == MOD_ZIPFILE)
    {
        QuaZip zip(m_file.filePath());
        if (!zip.open(QuaZip::mdUnzip))
            return;

        QuaZipFile file(&zip);

        if (zip.setCurrentFile("mcmod.info"))
        {
            if (!file.open(QIODevice::ReadOnly))
            {
                zip.close();
                return;
            }

            m_localDetails = ReadMCModInfo(file.readAll());
            file.close();
            zip.close();
            return;
        }
        else if (zip.setCurrentFile("fabric.mod.json"))
        {
            if (!file.open(QIODevice::ReadOnly))
            {
                zip.close();
                return;
            }

            m_localDetails = ReadFabricModInfo(file.readAll());
            file.close();
            zip.close();
            return;
        }
        else if (zip.setCurrentFile("forgeversion.properties"))
        {
            if (!file.open(QIODevice::ReadOnly))
            {
                zip.close();
                return;
            }

            m_localDetails = ReadForgeInfo(file.readAll());
            file.close();
            zip.close();
            return;
        }

        zip.close();
    }
    else if (m_type == MOD_FOLDER)
    {
        QFileInfo mcmod_info(FS::PathCombine(m_file.filePath(), "mcmod.info"));
        if (mcmod_info.isFile())
        {
            QFile mcmod(mcmod_info.filePath());
            if (!mcmod.open(QIODevice::ReadOnly))
                return;
            auto data = mcmod.readAll();
            if (data.isEmpty() || data.isNull())
                return;
            m_localDetails = ReadMCModInfo(data);
        }
    }
    else if (m_type == MOD_LITEMOD)
    {
        QuaZip zip(m_file.filePath());
        if (!zip.open(QuaZip::mdUnzip))
            return;

        QuaZipFile file(&zip);

        if (zip.setCurrentFile("litemod.json"))
        {
            if (!file.open(QIODevice::ReadOnly))
            {
                zip.close();
                return;
            }

            m_localDetails = ReadLiteModInfo(file.readAll());
            file.close();
        }
        zip.close();
    }
}

bool Mod::enable(bool value)
{
    if (m_type == Mod::MOD_UNKNOWN || m_type == Mod::MOD_FOLDER)
        return false;

    if (m_enabled == value)
        return false;

    QString path = m_file.absoluteFilePath();
    if (value)
    {
        QFile foo(path);
        if (!path.endsWith(".disabled"))
            return false;
        path.chop(9);
        if (!foo.rename(path))
            return false;
    }
    else
    {
        QFile foo(path);
        path += ".disabled";
        if (!foo.rename(path))
            return false;
    }
    m_file = QFileInfo(path);
    m_enabled = value;
    return true;
}

bool Mod::destroy()
{
    if (m_type == MOD_FOLDER)
    {
        QDir d(m_file.filePath());
        if (d.removeRecursively())
        {
            m_type = MOD_UNKNOWN;
            return true;
        }
        return false;
    }
    else if (m_type == MOD_SINGLEFILE || m_type == MOD_ZIPFILE || m_type == MOD_LITEMOD)
    {
        QFile f(m_file.filePath());
        if (f.remove())
        {
            m_type = MOD_UNKNOWN;
            return true;
        }
        return false;
    }
    return true;
}


const ModDetails & Mod::details() const
{
    if(!m_localDetails)
        return invalidDetails;
    return m_localDetails;
}


QString Mod::version() const
{
    return details().version;
}

QString Mod::name() const
{
    auto & d = details();
    if(d && !d.name.isEmpty()) {
        return d.name;
    }
    return m_name;
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
