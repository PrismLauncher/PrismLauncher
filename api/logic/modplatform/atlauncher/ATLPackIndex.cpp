#include "ATLPackIndex.h"

#include <QRegularExpression>

#include "Json.h"

static void loadIndexedVersion(ATLauncher::IndexedVersion & v, QJsonObject & obj)
{
    v.version = Json::requireString(obj, "version");
    v.minecraft = Json::requireString(obj, "minecraft");
}

void ATLauncher::loadIndexedPack(ATLauncher::IndexedPack & m, QJsonObject & obj)
{
    m.id = Json::requireInteger(obj, "id");
    m.position = Json::requireInteger(obj, "position");
    m.name = Json::requireString(obj, "name");
    m.type = Json::requireString(obj, "type") == "private" ?
            ATLauncher::PackType::Private :
            ATLauncher::PackType::Public;
    auto versionsArr = Json::requireArray(obj, "versions");
    for (const auto versionRaw : versionsArr)
    {
        auto versionObj = Json::requireObject(versionRaw);
        ATLauncher::IndexedVersion version;
        loadIndexedVersion(version, versionObj);
        m.versions.append(version);
    }
    m.system = Json::ensureBoolean(obj, "system", false);
    m.description = Json::ensureString(obj, "description", "");

    m.safeName = Json::requireString(obj, "name").replace(QRegularExpression("[^A-Za-z0-9]"), "");
}
