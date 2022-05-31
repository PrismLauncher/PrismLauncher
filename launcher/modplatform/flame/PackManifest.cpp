#include "PackManifest.h"
#include "Json.h"

static void loadFileV1(Flame::File& f, QJsonObject& file)
{
    f.projectId = Json::requireInteger(file, "projectID");
    f.fileId = Json::requireInteger(file, "fileID");
    f.required = Json::ensureBoolean(file, QString("required"), true);
}

static void loadModloaderV1(Flame::Modloader& m, QJsonObject& modLoader)
{
    m.id = Json::requireString(modLoader, "id");
    m.primary = Json::ensureBoolean(modLoader, QString("primary"), false);
}

static void loadMinecraftV1(Flame::Minecraft& m, QJsonObject& minecraft)
{
    m.version = Json::requireString(minecraft, "version");
    // extra libraries... apparently only used for a custom Minecraft launcher in the 1.2.5 FTB retro pack
    // intended use is likely hardcoded in the 'Flame' client, the manifest says nothing
    m.libraries = Json::ensureString(minecraft, QString("libraries"), QString());
    auto arr = Json::ensureArray(minecraft, "modLoaders", QJsonArray());
    for (QJsonValueRef item : arr) {
        auto obj = Json::requireObject(item);
        Flame::Modloader loader;
        loadModloaderV1(loader, obj);
        m.modLoaders.append(loader);
    }
}

static void loadManifestV1(Flame::Manifest& m, QJsonObject& manifest)
{
    auto mc = Json::requireObject(manifest, "minecraft");
    loadMinecraftV1(m.minecraft, mc);
    m.name = Json::ensureString(manifest, QString("name"), "Unnamed");
    m.version = Json::ensureString(manifest, QString("version"), QString());
    m.author = Json::ensureString(manifest, QString("author"), "Anonymous");
    auto arr = Json::ensureArray(manifest, "files", QJsonArray());
    for (QJsonValueRef item : arr) {
        auto obj = Json::requireObject(item);
        Flame::File file;
        loadFileV1(file, obj);
        m.files.insert(file.fileId,file);
    }
    m.overrides = Json::ensureString(manifest, "overrides", "overrides");
}

void Flame::loadManifest(Flame::Manifest& m, const QString& filepath)
{
    auto doc = Json::requireDocument(filepath);
    auto obj = Json::requireObject(doc);
    m.manifestType = Json::requireString(obj, "manifestType");
    if (m.manifestType != "minecraftModpack") {
        throw JSONValidationError("Not a modpack manifest!");
    }
    m.manifestVersion = Json::requireInteger(obj, "manifestVersion");
    if (m.manifestVersion != 1) {
        throw JSONValidationError(QString("Unknown manifest version (%1)").arg(m.manifestVersion));
    }
    loadManifestV1(m, obj);
}

bool Flame::File::parseFromObject(const QJsonObject& obj)
{
    fileName = Json::requireString(obj, "fileName");
    // This is a piece of a Flame project JSON pulled out into the file metadata (here) for convenience
    // It is also optional
    type = File::Type::SingleFile;

    if (fileName.endsWith(".zip")) {
        // this is probably a resource pack
        targetFolder = "resourcepacks";
    } else {
        // this is probably a mod, dunno what else could modpacks download
        targetFolder = "mods";
    }
    // get the hash
    hash = QString();
    auto hashes = Json::ensureArray(obj, "hashes");
    for(QJsonValueRef item : hashes) {
        auto hobj = Json::requireObject(item);
        auto algo = Json::requireInteger(hobj, "algo");
        auto value = Json::requireString(hobj, "value");
        if (algo == 1) {
            hash = value;
        }
    }


    // may throw, if the project is blocked
    QString rawUrl = Json::ensureString(obj, "downloadUrl");
    url = QUrl(rawUrl, QUrl::TolerantMode);
    if (!url.isValid()) {
        throw JSONValidationError(QString("Invalid URL: %1").arg(rawUrl));
    }

    resolved = true;
    return true;
}
