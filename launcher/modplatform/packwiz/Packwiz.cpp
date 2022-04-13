#include "Packwiz.h"

#include "modplatform/ModIndex.h"

#include <QDebug>
#include <QDir>
#include <QObject>

auto Packwiz::createModFormat(QDir& index_dir, ModPlatform::IndexedPack& mod_pack, ModPlatform::IndexedVersion& mod_version) -> Mod
{
    Mod mod;

    mod.name = mod_pack.name;
    mod.filename = mod_version.fileName;

    mod.url = mod_version.downloadUrl;
    mod.hash_format = ModPlatform::ProviderCapabilities::hashType(mod_pack.provider);
    mod.hash = "";  // FIXME

    mod.provider = mod_pack.provider;
    mod.file_id = mod_pack.addonId;
    mod.project_id = mod_version.fileId;

    return mod;
}

void Packwiz::updateModIndex(QDir& index_dir, Mod& mod)
{
    // Ensure the corresponding mod's info exists, and create it if not
    auto index_file_name = QString("%1.toml").arg(mod.name);
    QFile index_file(index_dir.absoluteFilePath(index_file_name));

    // There's already data on there!
    if (index_file.exists()) { index_file.remove(); }

    if (!index_file.open(QIODevice::ReadWrite)) {
        qCritical() << "Could not open file " << index_file_name << "!";
        return;
    }

    // Put TOML data into the file
    QTextStream in_stream(&index_file);
    auto addToStream = [&in_stream](QString&& key, QString value) { in_stream << QString("%1 = \"%2\"\n").arg(key, value); };

    {
        addToStream("name", mod.name);
        addToStream("filename", mod.filename);
        addToStream("side", mod.side);

        in_stream << QString("\n[download]\n");
        addToStream("url", mod.url.toString());
        addToStream("hash-format", mod.hash_format);
        addToStream("hash", mod.hash);

        in_stream << QString("\n[update]\n");
        in_stream << QString("[update.%1]\n").arg(ModPlatform::ProviderCapabilities::providerName(mod.provider));
        addToStream("file-id", mod.file_id.toString());
        addToStream("project-id", mod.project_id.toString());
    }
}
