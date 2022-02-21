/* Copyright 2020-2021 MultiMC Contributors
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

#include "TechnicPackProcessor.h"

#include <FileSystem.h>
#include <Json.h>
#include <minecraft/MinecraftInstance.h>
#include <minecraft/PackProfile.h>
#include <quazip/quazip.h>
#include <quazip/quazipdir.h>
#include <quazip/quazipfile.h>
#include <settings/INISettingsObject.h>

#include <memory>

void Technic::TechnicPackProcessor::run(SettingsObjectPtr globalSettings, const QString &instName, const QString &instIcon, const QString &stagingPath, const QString &minecraftVersion, const bool isSolder)
{
    QString minecraftPath = FS::PathCombine(stagingPath, ".minecraft");
    QString configPath = FS::PathCombine(stagingPath, "instance.cfg");
    auto instanceSettings = std::make_shared<INISettingsObject>(configPath);
    MinecraftInstance instance(globalSettings, instanceSettings, stagingPath);

    instance.setName(instName);

    if (instIcon != "default")
    {
        instance.setIconKey(instIcon);
    }

    auto components = instance.getPackProfile();
    components->buildingFromScratch();

    QByteArray data;

    QString modpackJar = FS::PathCombine(minecraftPath, "bin", "modpack.jar");
    QString versionJson = FS::PathCombine(minecraftPath, "bin", "version.json");
    QString fmlMinecraftVersion;
    if (QFile::exists(modpackJar))
    {
        QuaZip zipFile(modpackJar);
        if (!zipFile.open(QuaZip::mdUnzip))
        {
            emit failed(tr("Unable to open \"bin/modpack.jar\" file!"));
            return;
        }
        QuaZipDir zipFileRoot(&zipFile, "/");
        if (zipFileRoot.exists("/version.json"))
        {
            if (zipFileRoot.exists("/fmlversion.properties"))
            {
                zipFile.setCurrentFile("fmlversion.properties");
                QuaZipFile file(&zipFile);
                if (!file.open(QIODevice::ReadOnly))
                {
                    emit failed(tr("Unable to open \"fmlversion.properties\"!"));
                    return;
                }
                QByteArray fmlVersionData = file.readAll();
                file.close();
                INIFile iniFile;
                iniFile.loadFile(fmlVersionData);
                // If not present, this evaluates to a null string
                fmlMinecraftVersion = iniFile["fmlbuild.mcversion"].toString();
            }
            zipFile.setCurrentFile("version.json", QuaZip::csSensitive);
            QuaZipFile file(&zipFile);
            if (!file.open(QIODevice::ReadOnly))
            {
                emit failed(tr("Unable to open \"version.json\"!"));
                return;
            }
            data = file.readAll();
            file.close();
        }
        else
        {
            if (minecraftVersion.isEmpty())
                emit failed(tr("Could not find \"version.json\" inside \"bin/modpack.jar\", but minecraft version is unknown"));
            components->setComponentVersion("net.minecraft", minecraftVersion, true);
            components->installJarMods({modpackJar});

            // Forge for 1.4.7 and for 1.5.2 require extra libraries.
            // Figure out the forge version and add it as a component
            // (the code still comes from the jar mod installed above)
            if (zipFileRoot.exists("/forgeversion.properties"))
            {
                zipFile.setCurrentFile("forgeversion.properties", QuaZip::csSensitive);
                QuaZipFile file(&zipFile);
                if (!file.open(QIODevice::ReadOnly))
                {
                    // Really shouldn't happen, but error handling shall not be forgotten
                    emit failed(tr("Unable to open \"forgeversion.properties\""));
                    return;
                }
                QByteArray forgeVersionData = file.readAll();
                file.close();
                INIFile iniFile;
                iniFile.loadFile(forgeVersionData);
                QString major, minor, revision, build;
                major = iniFile["forge.major.number"].toString();
                minor = iniFile["forge.minor.number"].toString();
                revision = iniFile["forge.revision.number"].toString();
                build = iniFile["forge.build.number"].toString();

                if (major.isEmpty() || minor.isEmpty() || revision.isEmpty() || build.isEmpty())
                {
                    emit failed(tr("Invalid \"forgeversion.properties\"!"));
                    return;
                }

                components->setComponentVersion("net.minecraftforge", major + '.' + minor + '.' + revision + '.' + build);
            }

            components->saveNow();
            emit succeeded();
            return;
        }
    }
    else if (QFile::exists(versionJson))
    {
        QFile file(versionJson);
        if (!file.open(QIODevice::ReadOnly))
        {
            emit failed(tr("Unable to open \"version.json\"!"));
            return;
        }
        data = file.readAll();
        file.close();
    }
    else
    {
        // This is the "Vanilla" modpack, excluded by the search code
        emit failed(tr("Unable to find a \"version.json\"!"));
        return;
    }

    try
    {
        QJsonDocument doc = Json::requireDocument(data);
        QJsonObject root = Json::requireObject(doc, "version.json");
        QString minecraftVersion = Json::ensureString(root, "inheritsFrom", QString(), "");
        if (minecraftVersion.isEmpty())
        {
            if (fmlMinecraftVersion.isEmpty())
            {
                emit failed(tr("Could not understand \"version.json\":\ninheritsFrom is missing"));
                return;
            }
            minecraftVersion = fmlMinecraftVersion;
        }
        components->setComponentVersion("net.minecraft", minecraftVersion, true);
        for (auto library: Json::ensureArray(root, "libraries", {}))
        {
            if (!library.isObject())
            {
                continue;
            }

            auto libraryObject = Json::ensureObject(library, {}, "");
            auto libraryName = Json::ensureString(libraryObject, "name", "", "");

            if (libraryName.startsWith("net.minecraftforge:forge:") && libraryName.contains('-'))
            {
                QString libraryVersion = libraryName.section(':', 2);
                if (!libraryVersion.startsWith("1.7.10-"))
                {
                    components->setComponentVersion("net.minecraftforge", libraryName.section('-', 1));
                }
                else
                {
                    // 1.7.10 versions sometimes look like 1.7.10-10.13.4.1614-1.7.10, this filters out the 10.13.4.1614 part
                    components->setComponentVersion("net.minecraftforge", libraryName.section('-', 1, 1));
                }
            }
            else if (libraryName.startsWith("net.minecraftforge:minecraftforge:"))
            {
                components->setComponentVersion("net.minecraftforge", libraryName.section(':', 2));
            }
            else if (libraryName.startsWith("net.fabricmc:fabric-loader:"))
            {
                components->setComponentVersion("net.fabricmc.fabric-loader", libraryName.section(':', 2));
            }
        }
    }
    catch (const JSONValidationError &e)
    {
        emit failed(tr("Could not understand \"version.json\":\n") + e.cause());
        return;
    }

    components->saveNow();
    emit succeeded();
}
