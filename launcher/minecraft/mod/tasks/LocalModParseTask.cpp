#include "LocalModParseTask.h"

#include <quazip/quazip.h>
#include <quazip/quazipfile.h>
#include <toml++/toml.h>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>

#include "FileSystem.h"
#include "Json.h"
#include "settings/INIFile.h"

namespace {

// NEW format
// https://github.com/MinecraftForge/FML/wiki/FML-mod-information-file/6f62b37cea040daf350dc253eae6326dd9c822c3

// OLD format:
// https://github.com/MinecraftForge/FML/wiki/FML-mod-information-file/5bf6a2d05145ec79387acc0d45c958642fb049fc
ModDetails ReadMCModInfo(QByteArray contents)
{
    auto getInfoFromArray = [&](QJsonArray arr) -> ModDetails {
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

        for (auto author : authors) {
            details.authors.append(author.toString());
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

        int version = Json::ensureInteger(val, -1);

        // Some mods set the number with "", so it's a String instead
        if (version < 0)
            version = Json::ensureString(val, "").toInt();

        if (version != 2) {
            qCritical() << "BAD stuff happened to mod json:";
            qCritical() << contents;
            return {};
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

    // auto tomlData = toml::parse(contents.toStdString());
    toml::table tomlData;
#if TOML_EXCEPTIONS
    try {
        tomlData = toml::parse(contents.toStdString());
    } catch (const toml::parse_error& err) {
        return {};
    }
#else
    tomlData = toml::parse(contents.toStdString());
    if (!tomlData) {
        return {};
    }
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
    if (!tomlModsTable0) {
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
    } else if (auto authorsDatum = (*modsTable)["authors"].as_string()) {
        authors = QString::fromStdString(authorsDatum->get());
    }
    if (!authors.isEmpty()) {
        details.authors.append(authors);
    }

    QString homeurl = "";
    if (auto homeurlDatum = tomlData["displayURL"].as_string()) {
        homeurl = QString::fromStdString(homeurlDatum->get());
    } else if (auto homeurlDatum = (*modsTable)["displayURL"].as_string()) {
        homeurl = QString::fromStdString(homeurlDatum->get());
    }
    // fix up url.
    if (!homeurl.isEmpty() && !homeurl.startsWith("http://") && !homeurl.startsWith("https://") && !homeurl.startsWith("ftp://")) {
        homeurl.prepend("http://");
    }
    details.homeurl = homeurl;

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
        }
    }
    return details;
}

// https://github.com/QuiltMC/rfcs/blob/master/specification/0002-quilt.mod.json.md
ModDetails ReadQuiltModInfo(QByteArray contents)
{
    QJsonParseError jsonError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(contents, &jsonError);
    auto object = Json::requireObject(jsonDoc, "quilt.mod.json");
    auto schemaVersion = Json::ensureInteger(object.value("schema_version"), 0, "Quilt schema_version");

    ModDetails details;

    // https://github.com/QuiltMC/rfcs/blob/be6ba280d785395fefa90a43db48e5bfc1d15eb4/specification/0002-quilt.mod.json.md
    if (schemaVersion == 1) {
        auto modInfo = Json::requireObject(object.value("quilt_loader"), "Quilt mod info");

        details.mod_id = Json::requireString(modInfo.value("id"), "Mod ID");
        details.version = Json::requireString(modInfo.value("version"), "Mod version");

        auto modMetadata = Json::ensureObject(modInfo.value("metadata"));

        details.name = Json::ensureString(modMetadata.value("name"), details.mod_id);
        details.description = Json::ensureString(modMetadata.value("description"));

        auto modContributors = Json::ensureObject(modMetadata.value("contributors"));

        // We don't really care about the role of a contributor here
        details.authors += modContributors.keys();

        auto modContact = Json::ensureObject(modMetadata.value("contact"));

        if (modContact.contains("homepage")) {
            details.homeurl = Json::requireString(modContact.value("homepage"));
        }
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

}  // namespace

LocalModParseTask::LocalModParseTask(int token, ResourceType type, const QFileInfo& modFile)
    : Task(nullptr, false), m_token(token), m_type(type), m_modFile(modFile), m_result(new Result())
{}

void LocalModParseTask::processAsZip()
{
    QuaZip zip(m_modFile.filePath());
    if (!zip.open(QuaZip::mdUnzip))
        return;

    QuaZipFile file(&zip);

    if (zip.setCurrentFile("META-INF/mods.toml")) {
        if (!file.open(QIODevice::ReadOnly)) {
            zip.close();
            return;
        }

        m_result->details = ReadMCModTOML(file.readAll());
        file.close();

        // to replace ${file.jarVersion} with the actual version, as needed
        if (m_result->details.version == "${file.jarVersion}") {
            if (zip.setCurrentFile("META-INF/MANIFEST.MF")) {
                if (!file.open(QIODevice::ReadOnly)) {
                    zip.close();
                    return;
                }

                // quick and dirty line-by-line parser
                auto manifestLines = file.readAll().split('\n');
                QString manifestVersion = "";
                for (auto& line : manifestLines) {
                    if (QString(line).startsWith("Implementation-Version: ")) {
                        manifestVersion = QString(line).remove("Implementation-Version: ");
                        break;
                    }
                }

                // some mods use ${projectversion} in their build.gradle, causing this mess to show up in MANIFEST.MF
                // also keep with forge's behavior of setting the version to "NONE" if none is found
                if (manifestVersion.contains("task ':jar' property 'archiveVersion'") || manifestVersion == "") {
                    manifestVersion = "NONE";
                }

                m_result->details.version = manifestVersion;

                file.close();
            }
        }

        zip.close();
        return;
    } else if (zip.setCurrentFile("mcmod.info")) {
        if (!file.open(QIODevice::ReadOnly)) {
            zip.close();
            return;
        }

        m_result->details = ReadMCModInfo(file.readAll());
        file.close();
        zip.close();
        return;
    } else if (zip.setCurrentFile("quilt.mod.json")) {
        if (!file.open(QIODevice::ReadOnly)) {
            zip.close();
            return;
        }

        m_result->details = ReadQuiltModInfo(file.readAll());
        file.close();
        zip.close();
        return;
    } else if (zip.setCurrentFile("fabric.mod.json")) {
        if (!file.open(QIODevice::ReadOnly)) {
            zip.close();
            return;
        }

        m_result->details = ReadFabricModInfo(file.readAll());
        file.close();
        zip.close();
        return;
    } else if (zip.setCurrentFile("forgeversion.properties")) {
        if (!file.open(QIODevice::ReadOnly)) {
            zip.close();
            return;
        }

        m_result->details = ReadForgeInfo(file.readAll());
        file.close();
        zip.close();
        return;
    }

    zip.close();
}

void LocalModParseTask::processAsFolder()
{
    QFileInfo mcmod_info(FS::PathCombine(m_modFile.filePath(), "mcmod.info"));
    if (mcmod_info.isFile()) {
        QFile mcmod(mcmod_info.filePath());
        if (!mcmod.open(QIODevice::ReadOnly))
            return;
        auto data = mcmod.readAll();
        if (data.isEmpty() || data.isNull())
            return;
        m_result->details = ReadMCModInfo(data);
    }
}

void LocalModParseTask::processAsLitemod()
{
    QuaZip zip(m_modFile.filePath());
    if (!zip.open(QuaZip::mdUnzip))
        return;

    QuaZipFile file(&zip);

    if (zip.setCurrentFile("litemod.json")) {
        if (!file.open(QIODevice::ReadOnly)) {
            zip.close();
            return;
        }

        m_result->details = ReadLiteModInfo(file.readAll());
        file.close();
    }
    zip.close();
}

bool LocalModParseTask::abort()
{
    m_aborted.store(true);
    return true;
}

void LocalModParseTask::executeTask()
{
    switch (m_type) {
        case ResourceType::ZIPFILE:
            processAsZip();
            break;
        case ResourceType::FOLDER:
            processAsFolder();
            break;
        case ResourceType::LITEMOD:
            processAsLitemod();
            break;
        default:
            break;
    }

    if (m_aborted)
        emit finished();
    else
        emitSucceeded();
}
