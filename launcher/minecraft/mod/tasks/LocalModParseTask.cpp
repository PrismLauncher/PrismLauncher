#include "LocalModParseTask.h"

#include <qdcss.h>
#include <toml++/toml.h>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QRegularExpression>
#include <QString>

#include "FileSystem.h"
#include "Json.h"
#include "archive/ArchiveReader.h"
#include "minecraft/mod/ModDetails.h"
#include "settings/INIFile.h"

static const QRegularExpression s_newlineRegex("\r\n|\n|\r");

namespace ModUtils {

// NEW format
// https://github.com/MinecraftForge/FML/wiki/FML-mod-information-file/c8d8f1929aff9979e322af79a59ce81f3e02db6a

// OLD format:
// https://github.com/MinecraftForge/FML/wiki/FML-mod-information-file/5bf6a2d05145ec79387acc0d45c958642fb049fc
ModDetails ReadMCModInfo(QByteArray contents)
{
    auto getInfoFromArray = [](QJsonArray arr) -> ModDetails {
        if (!arr.at(0).isObject()) {
            return {};
        }
        ModDetails details;
        auto firstObj = arr.at(0).toObject();
        details.mod_id = firstObj.value("modid").toString();
        auto name = firstObj.value("name").toString();
        // NOTE: ignore stupid example mods copies where the author didn't even bother to change the name
        if (name != "Example Mod") {
            details.name = name;
        }
        details.version = firstObj.value("version").toString();
        auto homeurl = firstObj.value("url").toString().trimmed();
        if (!homeurl.isEmpty()) {
            // fix up url.
            if (!homeurl.startsWith("http://") && !homeurl.startsWith("https://") && !homeurl.startsWith("ftp://")) {
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

        if (firstObj.contains("logoFile")) {
            details.icon_file = firstObj.value("logoFile").toString();
        }

        for (auto author : authors) {
            details.authors.append(author.toString());
        }

        if (details.mod_id.startsWith("mod_")) {
            details.mod_id = details.mod_id.mid(4);
        }

        auto addDep = [&details](QString dep) {
            if (dep == "mod_MinecraftForge" || dep == "Forge")
                return;
            if (dep.contains(":")) {
                dep = dep.section(":", 1);
            }
            if (dep.contains("@")) {
                dep = dep.section("@", 0, 0);
            }
            if (dep.startsWith("mod_")) {
                dep = dep.mid(4);
            }
            details.dependencies.append(dep);
        };

        if (firstObj.contains("requiredMods")) {
            for (auto dep : firstObj.value("requiredMods").toArray()) {
                addDep(dep.toString());
            }
        } else if (firstObj.contains("dependencies")) {
            for (auto dep : firstObj.value("dependencies").toArray()) {
                addDep(dep.toString());
            }
        }

        return details;
    };
    QJsonParseError jsonError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(contents, &jsonError);
    // this is the very old format that had just the array
    if (jsonDoc.isArray()) {
        return getInfoFromArray(jsonDoc.array());
    } else if (jsonDoc.isObject()) {
        auto val = jsonDoc.object().value("modinfoversion");
        if (val.isUndefined()) {
            val = jsonDoc.object().value("modListVersion");
        }

        int version = val.toInt(-1);

        // Some mods set the number with "", so it's a String instead
        if (version < 0)
            version = val.toString("").toInt();

        if (version != 2) {
            qWarning() << QString(R"(The value of 'modListVersion' is "%1" (expected "2")! The file may be corrupted.)").arg(version);
            qWarning() << "The contents of 'mcmod.info' are as follows:";
            qWarning() << contents;
        }

        auto arrVal = jsonDoc.object().value("modlist");
        if (arrVal.isUndefined()) {
            arrVal = jsonDoc.object().value("modList");
        }
        if (arrVal.isArray()) {
            return getInfoFromArray(arrVal.toArray());
        }
    }
    return {};
}

// https://github.com/MinecraftForge/Documentation/blob/5ab4ba6cf9abc0ac4c0abd96ad187461aefd72af/docs/gettingstarted/structuring.md
ModDetails ReadMCModTOML(QByteArray contents)
{
    ModDetails details;

    toml::table tomlData;
#if TOML_EXCEPTIONS
    try {
        tomlData = toml::parse(contents.toStdString());
    } catch ([[maybe_unused]] const toml::parse_error& err) {
        return {};
    }
#else
    toml::parse_result result = toml::parse(contents.toStdString());
    if (!result) {
        return {};
    }
    tomlData = result.table();
#endif

    // array defined by [[mods]]
    auto tomlModsArr = tomlData["mods"].as_array();
    if (!tomlModsArr) {
        qWarning() << "Corrupted mods.toml? Couldn't find [[mods]] array!";
        return {};
    }

    // we only really care about the first element, since multiple mods in one file is not supported by us at the moment
    auto tomlModsTable0 = tomlModsArr->get(0);
    if (!tomlModsTable0) {
        qWarning() << "Corrupted mods.toml? [[mods]] didn't have an element at index 0!";
        return {};
    }
    auto modsTable = tomlModsTable0->as_table();
    if (!modsTable) {
        qWarning() << "Corrupted mods.toml? [[mods]] was not a table!";
        return {};
    }

    // mandatory properties - always in [[mods]]
    if (auto modIdDatum = (*modsTable)["modId"].as_string()) {
        details.mod_id = QString::fromStdString(modIdDatum->get());
    }
    if (auto versionDatum = (*modsTable)["version"].as_string()) {
        details.version = QString::fromStdString(versionDatum->get());
    }
    if (auto displayNameDatum = (*modsTable)["displayName"].as_string()) {
        details.name = QString::fromStdString(displayNameDatum->get());
    }
    if (auto descriptionDatum = (*modsTable)["description"].as_string()) {
        details.description = QString::fromStdString(descriptionDatum->get());
    }

    // optional properties - can be in the root table or [[mods]]
    QString authors = "";
    if (auto authorsDatum = tomlData["authors"].as_string()) {
        authors = QString::fromStdString(authorsDatum->get());
    } else if (auto authorsDatumMods = (*modsTable)["authors"].as_string()) {
        authors = QString::fromStdString(authorsDatumMods->get());
    }
    if (!authors.isEmpty()) {
        details.authors.append(authors);
    }

    QString homeurl = "";
    if (auto homeurlDatum = tomlData["displayURL"].as_string()) {
        homeurl = QString::fromStdString(homeurlDatum->get());
    } else if (auto homeurlDatumMods = (*modsTable)["displayURL"].as_string()) {
        homeurl = QString::fromStdString(homeurlDatumMods->get());
    }
    // fix up url.
    if (!homeurl.isEmpty() && !homeurl.startsWith("http://") && !homeurl.startsWith("https://") && !homeurl.startsWith("ftp://")) {
        homeurl.prepend("http://");
    }
    details.homeurl = homeurl;

    QString issueTrackerURL = "";
    if (auto issueTrackerURLDatum = tomlData["issueTrackerURL"].as_string()) {
        issueTrackerURL = QString::fromStdString(issueTrackerURLDatum->get());
    } else if (auto issueTrackerURLDatumMods = (*modsTable)["issueTrackerURL"].as_string()) {
        issueTrackerURL = QString::fromStdString(issueTrackerURLDatumMods->get());
    }
    details.issue_tracker = issueTrackerURL;

    QString license = "";
    if (auto licenseDatum = tomlData["license"].as_string()) {
        license = QString::fromStdString(licenseDatum->get());
    } else if (auto licenseDatumMods = (*modsTable)["license"].as_string()) {
        license = QString::fromStdString(licenseDatumMods->get());
    }
    if (!license.isEmpty())
        details.licenses.append(ModLicense(license));

    QString logoFile = "";
    if (auto logoFileDatum = tomlData["logoFile"].as_string()) {
        logoFile = QString::fromStdString(logoFileDatum->get());
    } else if (auto logoFileDatumMods = (*modsTable)["logoFile"].as_string()) {
        logoFile = QString::fromStdString(logoFileDatumMods->get());
    }
    details.icon_file = logoFile;

    auto parseDep = [&details](toml::array* dependencies) {
        static const QStringList ignoreModIds = { "", "forge", "neoforge", "minecraft" };
        if (!dependencies) {
            return;
        }
        auto isNeoForgeDep = [](toml::table* t) {
            auto type = (*t)["type"].as_string();
            return type && type->get() == "required";
        };
        auto isForgeDep = [](toml::table* t) {
            auto mandatory = (*t)["mandatory"].as_boolean();
            return mandatory && mandatory->get();
        };
        for (auto& dep : *dependencies) {
            auto dep_table = dep.as_table();
            if (!dep_table) {
                continue;
            }
            auto modId = (*dep_table)["modId"].as_string();
            if (!modId || ignoreModIds.contains(QString::fromStdString(modId->get()))) {
                continue;
            }
            if (isNeoForgeDep(dep_table) || isForgeDep(dep_table)) {
                details.dependencies.append(QString::fromStdString(modId->get()));
            }
        }
    };

    if (tomlData.contains("dependencies")) {
        auto depValue = tomlData["dependencies"];
        if (auto array = depValue.as_array()) {
            parseDep(array);
        } else if (auto depTable = depValue.as_table()) {
            auto expectedKey = details.mod_id.toStdString();
            if (!depTable->contains(expectedKey)) {
                for (auto [k, v] : *depTable) {
                    expectedKey = k;
                    break;
                }
            }
            if (auto array = (*depTable)[expectedKey].as_array()) {
                parseDep(array);
            }
        }
    }

    return details;
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

    if (schemaVersion >= 1) {
        QJsonArray authors = object.value("authors").toArray();
        for (auto author : authors) {
            if (author.isObject()) {
                details.authors.append(author.toObject().value("name").toString());
            } else {
                details.authors.append(author.toString());
            }
        }

        if (object.contains("contact")) {
            QJsonObject contact = object.value("contact").toObject();

            if (contact.contains("homepage")) {
                details.homeurl = contact.value("homepage").toString();
            }
            if (contact.contains("issues")) {
                details.issue_tracker = contact.value("issues").toString();
            }
        }

        if (object.contains("license")) {
            auto license = object.value("license");
            if (license.isArray()) {
                for (auto l : license.toArray()) {
                    if (l.isString()) {
                        details.licenses.append(ModLicense(l.toString()));
                    } else if (l.isObject()) {
                        auto obj = l.toObject();
                        details.licenses.append(ModLicense(obj.value("name").toString(), obj.value("id").toString(),
                                                           obj.value("url").toString(), obj.value("description").toString()));
                    }
                }
            } else if (license.isString()) {
                details.licenses.append(ModLicense(license.toString()));
            } else if (license.isObject()) {
                auto obj = license.toObject();
                details.licenses.append(ModLicense(obj.value("name").toString(), obj.value("id").toString(), obj.value("url").toString(),
                                                   obj.value("description").toString()));
            }
        }

        if (object.contains("icon")) {
            auto icon = object.value("icon");
            if (icon.isObject()) {
                auto obj = icon.toObject();
                // take the largest icon
                int largest = 0;
                for (auto key : obj.keys()) {
                    auto size = key.split('x').first().toInt();
                    if (size > largest) {
                        largest = size;
                    }
                }
                if (largest > 0) {
                    auto key = QString::number(largest) + "x" + QString::number(largest);
                    details.icon_file = obj.value(key).toString();
                } else {  // parsing the sizes failed
                    // take the first
                    for (auto i : obj) {
                        details.icon_file = i.toString();
                        break;
                    }
                }
            } else if (icon.isString()) {
                details.icon_file = icon.toString();
            }
        }

        if (object.contains("depends")) {
            auto depends = object.value("depends");
            if (depends.isObject()) {
                auto obj = depends.toObject();
                for (auto key : obj.keys()) {
                    if (key != "fabricloader" && key != "minecraft" && !key.startsWith("fabric-")) {
                        details.dependencies.append(key);
                    }
                }
            }
        }
    }
    return details;
}

// https://github.com/QuiltMC/rfcs/blob/master/specification/0002-quilt.mod.json.md
ModDetails ReadQuiltModInfo(QByteArray contents)
{
    ModDetails details;
    try {
        QJsonParseError jsonError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(contents, &jsonError);
        auto object = Json::requireObject(jsonDoc, "quilt.mod.json");
        auto schemaVersion = object.value("schema_version").toInt();

        // https://github.com/QuiltMC/rfcs/blob/be6ba280d785395fefa90a43db48e5bfc1d15eb4/specification/0002-quilt.mod.json.md
        if (schemaVersion == 1) {
            auto modInfo = Json::requireObject(object.value("quilt_loader"), "Quilt mod info");

            details.mod_id = Json::requireString(modInfo.value("id"), "Mod ID");
            details.version = Json::requireString(modInfo.value("version"), "Mod version");

            auto modMetadata = modInfo.value("metadata").toObject();

            details.name = modMetadata.value("name").toString(details.mod_id);
            details.description = modMetadata.value("description").toString();

            auto modContributors = modMetadata.value("contributors").toObject();

            // We don't really care about the role of a contributor here
            details.authors += modContributors.keys();

            auto modContact = modMetadata.value("contact").toObject();

            if (modContact.contains("homepage")) {
                details.homeurl = Json::requireString(modContact.value("homepage"));
            }
            if (modContact.contains("issues")) {
                details.issue_tracker = Json::requireString(modContact.value("issues"));
            }

            if (modMetadata.contains("license")) {
                auto license = modMetadata.value("license");
                if (license.isArray()) {
                    for (auto l : license.toArray()) {
                        if (l.isString()) {
                            details.licenses.append(ModLicense(l.toString()));
                        } else if (l.isObject()) {
                            auto obj = l.toObject();
                            details.licenses.append(ModLicense(obj.value("name").toString(), obj.value("id").toString(),
                                                               obj.value("url").toString(), obj.value("description").toString()));
                        }
                    }
                } else if (license.isString()) {
                    details.licenses.append(ModLicense(license.toString()));
                } else if (license.isObject()) {
                    auto obj = license.toObject();
                    details.licenses.append(ModLicense(obj.value("name").toString(), obj.value("id").toString(),
                                                       obj.value("url").toString(), obj.value("description").toString()));
                }
            }

            if (modMetadata.contains("icon")) {
                auto icon = modMetadata.value("icon");
                if (icon.isObject()) {
                    auto obj = icon.toObject();
                    // take the largest icon
                    int largest = 0;
                    for (auto key : obj.keys()) {
                        auto size = key.split('x').first().toInt();
                        if (size > largest) {
                            largest = size;
                        }
                    }
                    if (largest > 0) {
                        auto key = QString::number(largest) + "x" + QString::number(largest);
                        details.icon_file = obj.value(key).toString();
                    } else {  // parsing the sizes failed
                        // take the first
                        for (auto i : obj) {
                            details.icon_file = i.toString();
                            break;
                        }
                    }
                } else if (icon.isString()) {
                    details.icon_file = icon.toString();
                }
            }
            if (object.contains("depends")) {
                auto depends = object.value("depends");
                if (depends.isArray()) {
                    auto array = depends.toArray();
                    for (auto obj : array) {
                        QString modId;
                        if (obj.isString()) {
                            modId = obj.toString();
                        } else if (obj.isObject()) {
                            auto objValue = obj.toObject();
                            modId = objValue.value("id").toString();
                            if (objValue.contains("optional") && objValue.value("optional").toBool()) {
                                continue;
                            }
                        } else {
                            continue;
                        }
                        if (modId != "minecraft" && !modId.startsWith("quilt_")) {
                            details.dependencies.append(modId);
                        }
                    }
                }
            }
        }

    } catch (const Exception& e) {
        qWarning() << "Unable to parse mod info:" << e.cause();
    }
    return details;
}

ModDetails ReadForgeInfo(QByteArray contents)
{
    ModDetails details;
    // Read the data
    details.name = "Minecraft Forge";
    details.mod_id = "Forge";
    details.homeurl = "http://www.minecraftforge.net/forum/";
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
    if (object.contains("name")) {
        details.mod_id = details.name = object.value("name").toString();
    }
    if (object.contains("version")) {
        details.version = object.value("version").toString("");
    } else {
        details.version = object.value("revision").toString("");
    }
    details.mcversion = object.value("mcversion").toString();
    auto author = object.value("author").toString();
    if (!author.isEmpty()) {
        details.authors.append(author);
    }
    details.description = object.value("description").toString();
    details.homeurl = object.value("url").toString();
    return details;
}

// https://git.sleeping.town/unascribed/NilLoader/src/commit/d7fc87b255fc31019ff90f80d45894927fac6efc/src/main/java/nilloader/api/NilMetadata.java#L64
ModDetails ReadNilModInfo(QByteArray contents, QString fname)
{
    ModDetails details;

    QDCSS cssData = QDCSS(contents);
    auto name = cssData.get("@nilmod.name");
    auto desc = cssData.get("@nilmod.description");
    auto authors = cssData.get("@nilmod.authors");

    if (name->has_value()) {
        details.name = name->value();
    }
    if (desc->has_value()) {
        details.description = desc->value();
    }
    if (authors->has_value()) {
        details.authors.append(authors->value());
    }
    details.version = cssData.get("@nilmod.version")->value_or("?");

    details.mod_id = fname.remove(".nilmod.css");

    return details;
}

bool process(Mod& mod, ProcessingLevel level)
{
    switch (mod.type()) {
        case ResourceType::FOLDER:
            return processFolder(mod, level);
        case ResourceType::ZIPFILE:
            return processZIP(mod, level);
        case ResourceType::LITEMOD:
            return processLitemod(mod);
        default:
            qWarning() << "Invalid type for mod parse task!";
            return false;
    }
}

bool processZIP(Mod& mod, [[maybe_unused]] ProcessingLevel level)
{
    ModDetails details;

    MMCZip::ArchiveReader zip(mod.fileinfo().filePath());

    bool baseForgePopulated = false;
    bool isNilMod = false;
    bool isValid = false;
    QString manifestVersion = {};
    QByteArray nilData = {};
    QString nilFilePath = {};

    if (!zip.parse([&details, &baseForgePopulated, &manifestVersion, &isValid, &nilData, &isNilMod, &nilFilePath](
                       MMCZip::ArchiveReader::File* file, bool& stop) {
            auto filePath = file->filename();

            if (filePath == "META-INF/mods.toml" || filePath == "META-INF/neoforge.mods.toml") {
                details = ReadMCModTOML(file->readAll());
                isValid = true;
                if (details.version == "${file.jarVersion}" && !manifestVersion.isEmpty()) {
                    details.version = manifestVersion;
                }
                stop = details.version != "${file.jarVersion}";
                baseForgePopulated = true;
                return true;
            }
            if (filePath == "META-INF/MANIFEST.MF") {
                // quick and dirty line-by-line parser
                auto manifestLines = QString(file->readAll()).split(s_newlineRegex);
                manifestVersion = "";
                for (auto& line : manifestLines) {
                    if (line.startsWith("Implementation-Version: ", Qt::CaseInsensitive)) {
                        manifestVersion = line.remove("Implementation-Version: ", Qt::CaseInsensitive);
                        break;
                    }
                }

                // some mods use ${projectversion} in their build.gradle, causing this mess to show up in MANIFEST.MF
                // also keep with forge's behavior of setting the version to "NONE" if none is found
                if (manifestVersion.contains("task ':jar' property 'archiveVersion'") || manifestVersion == "") {
                    manifestVersion = "NONE";
                }
                if (baseForgePopulated) {
                    details.version = manifestVersion;
                    stop = true;
                }
                return true;
            }
            if (filePath == "mcmod.info") {
                details = ReadMCModInfo(file->readAll());
                isValid = true;
                stop = true;
                return true;
            }
            if (filePath == "quilt.mod.json") {
                details = ReadQuiltModInfo(file->readAll());
                isValid = true;
                stop = true;
                return true;
            }
            if (filePath == "fabric.mod.json") {
                details = ReadFabricModInfo(file->readAll());
                isValid = true;
                stop = true;
                return true;
            }
            if (filePath == "forgeversion.properties") {
                details = ReadForgeInfo(file->readAll());
                isValid = true;
                stop = true;
                return true;
            }
            if (filePath == "META-INF/nil/mappings.json") {
                // nilloader uses the filename of the metadata file for the modid, so we can't know the exact filename
                // thankfully, there is a good file to use as a canary so we don't look for nil meta all the time
                isNilMod = true;
                stop = !nilFilePath.isEmpty();
                file->skip();
                return true;
            }
            // nilmods can shade nilloader to be able to run as a standalone agent - which includes nilloader's own meta file
            if (filePath.endsWith(".nilmod.css") && filePath != "nilloader.nilmod.css") {
                nilData = file->readAll();
                nilFilePath = filePath;
                stop = isNilMod;
                return true;
            }
            file->skip();
            return true;
        })) {
        return false;
    }
    if (isNilMod) {
        details = ReadNilModInfo(nilData, nilFilePath);
        isValid = true;
    }
    if (isValid) {
        mod.setDetails(details);
        return true;
    }
    return false;  // no valid mod found in archive
}

bool processFolder(Mod& mod, [[maybe_unused]] ProcessingLevel level)
{
    ModDetails details;

    QFileInfo mcmod_info(FS::PathCombine(mod.fileinfo().filePath(), "mcmod.info"));
    if (mcmod_info.exists() && mcmod_info.isFile()) {
        QFile mcmod(mcmod_info.filePath());
        if (!mcmod.open(QIODevice::ReadOnly))
            return false;
        auto data = mcmod.readAll();
        if (data.isEmpty() || data.isNull())
            return false;
        details = ReadMCModInfo(data);

        mod.setDetails(details);
        return true;
    }

    return false;  // no valid mcmod.info file found
}

bool processLitemod(Mod& mod, [[maybe_unused]] ProcessingLevel level)
{
    ModDetails details;

    MMCZip::ArchiveReader zip(mod.fileinfo().filePath());

    if (auto file = zip.goToFile("litemod.json"); file) {
        details = ReadLiteModInfo(file->readAll());

        mod.setDetails(details);
        return true;
    }

    return false;  // no valid litemod.json found in archive
}

/** Checks whether a file is valid as a mod or not. */
bool validate(QFileInfo file)
{
    Mod mod{ file };
    return ModUtils::process(mod, ProcessingLevel::BasicInfoOnly) && mod.valid();
}

bool processIconPNG(const Mod& mod, QByteArray&& raw_data, QPixmap* pixmap)
{
    auto img = QImage::fromData(raw_data);
    if (!img.isNull()) {
        *pixmap = mod.setIcon(img);
    } else {
        qWarning() << "Failed to parse mod logo:" << mod.iconPath() << "from" << mod.name();
        return false;
    }
    return true;
}

bool loadIconFile(const Mod& mod, QPixmap* pixmap)
{
    if (mod.iconPath().isEmpty()) {
        qWarning() << "No Iconfile set, be sure to parse the mod first";
        return false;
    }

    auto png_invalid = [&mod](const QString& reason) {
        qWarning() << "Mod at" << mod.fileinfo().filePath() << "does not have a valid icon:" << reason;
        return false;
    };

    switch (mod.type()) {
        case ResourceType::FOLDER: {
            QFileInfo icon_info(FS::PathCombine(mod.fileinfo().filePath(), mod.iconPath()));
            if (icon_info.exists() && icon_info.isFile()) {
                QFile icon(icon_info.filePath());
                if (!icon.open(QIODevice::ReadOnly)) {
                    return png_invalid("failed  to open file " + icon_info.filePath());
                }
                auto data = icon.readAll();

                bool icon_result = ModUtils::processIconPNG(mod, std::move(data), pixmap);

                icon.close();

                if (!icon_result) {
                    return png_invalid("invalid png image");  // icon invalid
                }
                return true;
            }
            return png_invalid("file '" + icon_info.filePath() + "' does not exists or is not a file");
        }
        case ResourceType::ZIPFILE: {
            MMCZip::ArchiveReader zip(mod.fileinfo().filePath());
            auto file = zip.goToFile(mod.iconPath());
            if (file) {
                auto data = file->readAll();

                bool icon_result = ModUtils::processIconPNG(mod, std::move(data), pixmap);

                if (!icon_result) {
                    return png_invalid("invalid png image");  // icon png invalid
                }
                return true;
            }
            return png_invalid("Failed to set '" + mod.iconPath() +
                               "' as current file in zip archive");  // could not set icon as current file.
        }
        case ResourceType::LITEMOD: {
            return png_invalid("litemods do not have icons");  // can lightmods even have icons?
        }
        default:
            return png_invalid("Invalid type for mod, can not load icon.");
    }
}

}  // namespace ModUtils

LocalModParseTask::LocalModParseTask(int token, ResourceType type, const QFileInfo& modFile)
    : Task(false), m_token(token), m_type(type), m_modFile(modFile), m_result(new Result())
{}

bool LocalModParseTask::abort()
{
    m_aborted.store(true);
    return true;
}

void LocalModParseTask::executeTask()
{
    Mod mod{ m_modFile };
    ModUtils::process(mod, ModUtils::ProcessingLevel::Full);

    m_result->details = mod.details();

    if (m_aborted)
        emitAborted();
    else
        emitSucceeded();
}
