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

static void loadManifestV1(Flame::Manifest& pack, QJsonObject& manifest)
{
    auto mc = Json::requireObject(manifest, "minecraft");

    loadMinecraftV1(pack.minecraft, mc);

    pack.name = Json::ensureString(manifest, QString("name"), "Unnamed");
    pack.version = Json::ensureString(manifest, QString("version"), QString());
    pack.author = Json::ensureString(manifest, QString("author"), "Anonymous");

    auto arr = Json::ensureArray(manifest, "files", QJsonArray());
    for (auto item : arr) {
        auto obj = Json::requireObject(item);

        Flame::File file;
        loadFileV1(file, obj);

        pack.files.insert(file.fileId, file);
    }

    pack.overrides = Json::ensureString(manifest, "overrides", "overrides");

    pack.is_loaded = true;
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
