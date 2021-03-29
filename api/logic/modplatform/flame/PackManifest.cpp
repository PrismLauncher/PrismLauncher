#include "PackManifest.h"
#include "Json.h"

static void loadFileV1(Flame::File & f, QJsonObject & file)
{
    f.projectId = Json::requireInteger(file, "projectID");
    f.fileId = Json::requireInteger(file, "fileID");
    f.required = Json::ensureBoolean(file, QString("required"), true);
}

static void loadModloaderV1(Flame::Modloader & m, QJsonObject & modLoader)
{
    m.id = Json::requireString(modLoader, "id");
    m.primary = Json::ensureBoolean(modLoader, QString("primary"), false);
}

static void loadMinecraftV1(Flame::Minecraft & m, QJsonObject & minecraft)
{
    m.version = Json::requireString(minecraft, "version");
    // extra libraries... apparently only used for a custom Minecraft launcher in the 1.2.5 FTB retro pack
    // intended use is likely hardcoded in the 'Flame' client, the manifest says nothing
    m.libraries = Json::ensureString(minecraft, QString("libraries"), QString());
    auto arr = Json::ensureArray(minecraft, "modLoaders", QJsonArray());
    for (QJsonValueRef item : arr)
    {
        auto obj = Json::requireObject(item);
        Flame::Modloader loader;
        loadModloaderV1(loader, obj);
        m.modLoaders.append(loader);
    }
}

static void loadManifestV1(Flame::Manifest & m, QJsonObject & manifest)
{
    auto mc = Json::requireObject(manifest, "minecraft");
    loadMinecraftV1(m.minecraft, mc);
    m.name = Json::ensureString(manifest, QString("name"), "Unnamed");
    m.version = Json::ensureString(manifest, QString("version"), QString());
    m.author = Json::ensureString(manifest, QString("author"), "Anonymous Coward");
    auto arr = Json::ensureArray(manifest, "files", QJsonArray());
    for (QJsonValueRef item : arr)
    {
        auto obj = Json::requireObject(item);
        Flame::File file;
        loadFileV1(file, obj);
        m.files.append(file);
    }
    m.overrides = Json::ensureString(manifest, "overrides", "overrides");
}

void Flame::loadManifest(Flame::Manifest & m, const QString &filepath)
{
    auto doc = Json::requireDocument(filepath);
    auto obj = Json::requireObject(doc);
    m.manifestType = Json::requireString(obj, "manifestType");
    if(m.manifestType != "minecraftModpack")
    {
        throw JSONValidationError("Not a modpack manifest!");
    }
    m.manifestVersion = Json::requireInteger(obj, "manifestVersion");
    if(m.manifestVersion != 1)
    {
        throw JSONValidationError(QString("Unknown manifest version (%1)").arg(m.manifestVersion));
    }
    loadManifestV1(m, obj);
}

bool Flame::File::parseFromBytes(const QByteArray& bytes)
{
    auto doc = Json::requireDocument(bytes);
    auto obj = Json::requireObject(doc);
    // result code signifies true failure.
    if(obj.contains("code"))
    {
        qCritical() << "Resolving of" << projectId << fileId << "failed because of a negative result:";
        qCritical() << bytes;
        return false;
    }
    fileName = Json::requireString(obj, "FileNameOnDisk");
    QString rawUrl = Json::requireString(obj, "DownloadURL");
    url = QUrl(rawUrl, QUrl::TolerantMode);
    if(!url.isValid())
    {
        throw JSONValidationError(QString("Invalid URL: %1").arg(rawUrl));
    }
    // This is a piece of a Flame project JSON pulled out into the file metadata (here) for convenience
    // It is also optional
    QJsonObject projObj = Json::ensureObject(obj, "_Project", {});
    if(!projObj.isEmpty())
    {
        QString strType = Json::ensureString(projObj, "PackageType", "mod").toLower();
        if(strType == "singlefile")
        {
            type = File::Type::SingleFile;
        }
        else if(strType == "ctoc")
        {
            type = File::Type::Ctoc;
        }
        else if(strType == "cmod2")
        {
            type = File::Type::Cmod2;
        }
        else if(strType == "mod")
        {
            type = File::Type::Mod;
        }
        else if(strType == "folder")
        {
            type = File::Type::Folder;
        }
        else if(strType == "modpack")
        {
            type = File::Type::Modpack;
        }
        else
        {
            qCritical() << "Resolving of" << projectId << fileId << "failed because of unknown file type:" << strType;
            type = File::Type::Unknown;
            return false;
        }
        targetFolder = Json::ensureString(projObj, "Path", "mods");
    }
    resolved = true;
    return true;
}
