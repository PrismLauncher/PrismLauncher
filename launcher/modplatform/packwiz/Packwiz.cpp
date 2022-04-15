#include "Packwiz.h"

#include "modplatform/ModIndex.h"
#include "toml.h"

#include <QDebug>
#include <QDir>
#include <QObject>

// Helpers
static inline QString indexFileName(QString const& mod_name)
{
    return QString("%1.toml").arg(mod_name);
}

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
    QFile index_file(index_dir.absoluteFilePath(indexFileName(mod.name)));

    // There's already data on there!
    if (index_file.exists()) { index_file.remove(); }

    if (!index_file.open(QIODevice::ReadWrite)) {
        qCritical() << QString("Could not open file %1!").arg(indexFileName(mod.name));
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

void Packwiz::deleteModIndex(QDir& index_dir, QString& mod_name)
{
    QFile index_file(index_dir.absoluteFilePath(indexFileName(mod_name)));

    if(!index_file.exists()){
        qWarning() << QString("Tried to delete non-existent mod metadata for %1!").arg(mod_name);
        return;
    }

    if(!index_file.remove()){
        qWarning() << QString("Failed to remove metadata for mod %1!").arg(mod_name);
    }
}

auto Packwiz::getIndexForMod(QDir& index_dir, QString& mod_name) -> Mod
{
    Mod mod;

    QFile index_file(index_dir.absoluteFilePath(indexFileName(mod_name)));

    if (!index_file.exists()) {
        qWarning() << QString("Tried to get a non-existent mod metadata for %1").arg(mod_name);
        return mod;
    }
    if (!index_file.open(QIODevice::ReadOnly)) {
        qWarning() << QString("Failed to open mod metadata for %1").arg(mod_name);
        return mod;
    }

    toml_table_t* table;

    char errbuf[200];
    table = toml_parse(index_file.readAll().data(), errbuf, sizeof(errbuf));

    index_file.close();

    if (!table) {
        qCritical() << QString("Could not open file %1!").arg(indexFileName(mod.name));
        return mod;
    }

    // Helper function for extracting data from the TOML file
    auto stringEntry = [&](toml_table_t* parent, const char* entry_name) -> QString {
        toml_datum_t var = toml_string_in(parent, entry_name);
        if (!var.ok) {
            qCritical() << QString("Failed to read property '%1' in mod metadata.").arg(entry_name);
            return {};
        }

        QString tmp = var.u.s;
        free(var.u.s);

        return tmp;
    };

    { // Basic info
        mod.name = stringEntry(table, "name");
        // Basic sanity check
        if (mod.name != mod_name) {
            qCritical() << QString("Name mismatch in mod metadata:\nExpected:%1\nGot:%2").arg(mod_name, mod.name);
            return {};
        }

        mod.filename = stringEntry(table, "filename");
        mod.side = stringEntry(table, "side");
    }

    { // [download] info
        toml_table_t* download_table = toml_table_in(table, "download");
        if (!download_table) {
            qCritical() << QString("No [download] section found on mod metadata!");
            return {};
        }

        mod.url = stringEntry(download_table, "url");
        mod.hash_format = stringEntry(download_table, "hash-format");
        mod.hash = stringEntry(download_table, "hash");
    }

    { // [update] info
        using ProviderCaps = ModPlatform::ProviderCapabilities;
        using Provider = ModPlatform::Provider;

        toml_table_t* update_table = toml_table_in(table, "update");
        if (!update_table) {
            qCritical() << QString("No [update] section found on mod metadata!");
            return {};
        }

        toml_table_t* mod_provider_table;
        if ((mod_provider_table = toml_table_in(update_table, ProviderCaps::providerName(Provider::FLAME)))) {
            mod.provider = Provider::FLAME;
        } else if ((mod_provider_table = toml_table_in(update_table, ProviderCaps::providerName(Provider::MODRINTH)))) {
            mod.provider = Provider::MODRINTH;
        } else {
            qCritical() << QString("No mod provider on mod metadata!");
            return {};
        }
        
        mod.file_id = stringEntry(mod_provider_table, "file-id");
        mod.project_id = stringEntry(mod_provider_table, "project-id");
    }

    toml_free(table);

    return mod;
}
