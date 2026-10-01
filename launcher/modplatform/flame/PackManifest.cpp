#include "PackManifest.h"
#include "Json.h"

namespace {
Result<> loadFileV1(Flame::File& f, const QJsonObject& file)
{
    TRY_INTO(f.projectId, Json::requireInteger(file, "projectID"))
    TRY_INTO(f.fileId, Json::requireInteger(file, "fileID"))
    f.required = file["required"].toBool(true);
    return {};
}

Result<> loadModloaderV1(Flame::Modloader& m, const QJsonObject& modLoader)
{
    TRY_INTO(m.id, Json::requireString(modLoader, "id"))
    m.primary = modLoader["primary"].toBool();
    return {};
}

Result<> loadMinecraftV1(Flame::Minecraft& m, const QJsonObject& minecraft)
{
    TRY_INTO(m.version, Json::requireString(minecraft, "version"))
    // extra libraries... apparently only used for a custom Minecraft launcher in the 1.2.5 FTB retro pack
    // intended use is likely hardcoded in the 'Flame' client, the manifest says nothing
    m.libraries = minecraft["libraries"].toString();
    auto arr = minecraft["modLoaders"].toArray();
    for (const auto& item : arr) {
        Flame::Modloader loader;
        TRY(Json::requireObject(item).and_then([&loader](const auto& v) { return loadModloaderV1(loader, v); }))
        m.modLoaders.append(loader);
    }
    m.recommendedRAM = minecraft["recommendedRam"].toInt();
    return {};
}

Result<> loadManifestV1(Flame::Manifest& pack, const QJsonObject& manifest)
{
    TRY(Json::requireObject(manifest, "minecraft").and_then([&pack](const auto& v) { return loadMinecraftV1(pack.minecraft, v); }))

    pack.name = manifest["name"].toString("Unnamed");
    pack.version = manifest["version"].toString();
    pack.author = manifest["author"].toString("Anonymous");

    auto arr = manifest["files"].toArray();
    for (auto item : arr) {
        Flame::File file;
        TRY(Json::requireObject(item).and_then([&file](const auto& v) { return loadFileV1(file, v); }))
        Q_ASSERT(file.projectId != 0);
        pack.files.insert(file.fileId, file);
    }

    pack.overrides = manifest["overrides"].toString("overrides");

    pack.isLoaded = true;
    return {};
}
}  // namespace

Result<> Flame::loadManifest(Flame::Manifest& m, const QString& filepath)
{
    TRY_INTO(const auto& obj, Json::requireObject(filepath))
    TRY_INTO(m.manifestType, Json::requireString(obj, "manifestType"))
    if (m.manifestType != "minecraftModpack") {
        return std::unexpected("Not a modpack manifest!");
    }
    TRY_INTO(m.manifestVersion, Json::requireInteger(obj, "manifestVersion"))
    if (m.manifestVersion != 1) {
        return std::unexpected(QString("Unknown manifest version (%1)").arg(m.manifestVersion));
    }
    TRY(loadManifestV1(m, obj))
    return {};
}
