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
#include <settings/INISettingsObject.h>

#include <memory>
#include "archive/ArchiveReader.h"

void Technic::TechnicPackProcessor::run(SettingsObject* globalSettings,
                                        const QString& instName,
                                        const QString& instIcon,
                                        const QString& stagingPath,
                                        const QString& minecraftVersion,
                                        [[maybe_unused]] const bool isSolder)
{
    QString minecraftPath = FS::PathCombine(stagingPath, "minecraft");
    QString configPath = FS::PathCombine(stagingPath, "instance.cfg");
    auto instanceSettings = std::make_unique<INISettingsObject>(configPath);
    MinecraftInstance instance(globalSettings, std::move(instanceSettings), stagingPath);

    instance.setName(instName);

    if (instIcon != "default") {
        instance.setIconKey(instIcon);
    }

    auto components = instance.getPackProfile();
    components->buildingFromScratch();

    QByteArray data;

    QString modpackJar = FS::PathCombine(minecraftPath, "bin", "modpack.jar");
    QString versionJson = FS::PathCombine(minecraftPath, "bin", "version.json");
    QString fmlMinecraftVersion;
    if (QFile::exists(modpackJar)) {
        MMCZip::ArchiveReader zipFile(modpackJar);
        if (!zipFile.collectFiles()) {
            emit failed(tr("Unable to open \"bin/modpack.jar\" file!"));
            return;
        }
        if (zipFile.exists("/version.json")) {
            if (zipFile.exists("/fmlversion.properties")) {
                auto file = zipFile.goToFile("fmlversion.properties");
                if (!file) {
                    emit failed(tr("Unable to open \"fmlversion.properties\"!"));
                    return;
                }
                QByteArray fmlVersionData = file->readAll();
                INIFile iniFile;
                iniFile.loadFile(fmlVersionData);
                // If not present, this evaluates to a null string
                fmlMinecraftVersion = iniFile["fmlbuild.mcversion"].toString();
            }
            auto file = zipFile.goToFile("version.json");
            if (!file) {
                emit failed(tr("Unable to open \"version.json\"!"));
                return;
            }
            data = file->readAll();
        } else {
            if (minecraftVersion.isEmpty()) {
                emit failed(tr("Could not find \"version.json\" inside \"bin/modpack.jar\", but Minecraft version is unknown"));
                return;
            }
            components->setComponentVersion("net.minecraft", minecraftVersion, true);
            components->installJarMods({ modpackJar });

            // Forge for 1.4.7 and for 1.5.2 require extra libraries.
            // Figure out the forge version and add it as a component
            // (the code still comes from the jar mod installed above)
            if (zipFile.exists("/forgeversion.properties")) {
                auto file = zipFile.goToFile("forgeversion.properties");
                if (!file) {
                    // Really shouldn't happen, but error handling shall not be forgotten
                    emit failed(tr("Unable to open \"forgeversion.properties\""));
                    return;
                }
                auto forgeVersionData = file->readAll();
                INIFile iniFile;
                iniFile.loadFile(forgeVersionData);
                QString major, minor, revision, build;
                major = iniFile["forge.major.number"].toString();
                minor = iniFile["forge.minor.number"].toString();
                revision = iniFile["forge.revision.number"].toString();
                build = iniFile["forge.build.number"].toString();

                if (major.isEmpty() || minor.isEmpty() || revision.isEmpty() || build.isEmpty()) {
                    emit failed(tr("Invalid \"forgeversion.properties\"!"));
                    return;
                }

                components->setComponentVersion("net.minecraftforge", major + '.' + minor + '.' + revision + '.' + build);
            }

            components->saveNow();
            emit succeeded();
            return;
        }
    } else if (QFile::exists(versionJson)) {
        QFile file(versionJson);
        if (!file.open(QIODevice::ReadOnly)) {
            emit failed(tr("Unable to open \"version.json\"!"));
            return;
        }
        data = file.readAll();
        file.close();
    } else {
        // This is the "Vanilla" modpack, excluded by the search code
        components->setComponentVersion("net.minecraft", minecraftVersion, true);
        components->saveNow();
        emit succeeded();
        return;
    }

    try {
        QJsonDocument doc = Json::requireDocument(data);
        QJsonObject root = Json::requireObject(doc, "version.json");
        QString packMinecraftVersion = root["inheritsFrom"].toString();
        if (packMinecraftVersion.isEmpty()) {
            if (fmlMinecraftVersion.isEmpty()) {
                emit failed(tr("Could not understand \"version.json\":\ninheritsFrom is missing"));
                return;
            }
            packMinecraftVersion = fmlMinecraftVersion;
        }
        components->setComponentVersion("net.minecraft", packMinecraftVersion, true);
        for (auto library : root["libraries"].toArray()) {
            if (!library.isObject()) {
                continue;
            }

            auto libraryObject = library.toObject();
            auto libraryName = libraryObject["name"].toString();

            if (libraryName.startsWith("net.neoforged.fancymodloader:")) {  // it is neoforge
                // no easy way to get the version from the libs so use the arguments
                auto arguments = root["arguments"].toObject();
                bool isVersionArg = false;
                QString neoforgeVersion;
                for (auto arg : arguments["game"].toArray()) {
                    auto argument = arg.toString("");
                    if (isVersionArg) {
                        neoforgeVersion = argument;
                        break;
                    } else {
                        isVersionArg = "--fml.neoForgeVersion" == argument || "--fml.forgeVersion" == argument;
                    }
                }
                if (!neoforgeVersion.isEmpty()) {
                    components->setComponentVersion("net.neoforged", neoforgeVersion);
                }
                break;
            } else if ((libraryName.startsWith("net.minecraftforge:forge:") || libraryName.startsWith("net.minecraftforge:fmlloader:")) &&
                       libraryName.contains('-')) {
                QString libraryVersion = libraryName.section(':', 2);
                if (!libraryVersion.startsWith("1.7.10-")) {
                    components->setComponentVersion("net.minecraftforge", libraryName.section('-', 1));
                } else {
                    // 1.7.10 versions sometimes look like 1.7.10-10.13.4.1614-1.7.10, this filters out the 10.13.4.1614 part
                    components->setComponentVersion("net.minecraftforge", libraryName.section('-', 1, 1));
                }
                break;
            } else {
                // <Technic library name prefix> -> <our component name>
                static QMap<QString, QString> loaderMap{ { "net.minecraftforge:minecraftforge:", "net.minecraftforge" },
                                                         { "net.fabricmc:fabric-loader:", "net.fabricmc.fabric-loader" },
                                                         { "org.quiltmc:quilt-loader:", "org.quiltmc.quilt-loader" } };
                for (const auto& loader : loaderMap.keys()) {
                    if (libraryName.startsWith(loader)) {
                        components->setComponentVersion(loaderMap.value(loader), libraryName.section(':', 2));
                        break;
                    }
                }
            }
        }
    } catch (const JSONValidationError& e) {
        emit failed(tr("Could not understand \"version.json\":\n") + e.cause());
        return;
    }

    components->saveNow();
    emit succeeded();
}
