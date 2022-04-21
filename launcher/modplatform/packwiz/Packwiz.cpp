#include "Packwiz.h"

#include <QDebug>
#include <QDir>
#include <QObject>

#include "toml.h"

#include "minecraft/mod/Mod.h"
#include "modplatform/ModIndex.h"

namespace Packwiz {

// Helpers
static inline auto indexFileName(QString const& mod_name) -> QString
{
    if(mod_name.endsWith(".toml"))
        return mod_name;
    return QString("%1.toml").arg(mod_name);
}

static ModPlatform::ProviderCapabilities ProviderCaps;

auto V1::createModFormat(QDir& index_dir, ModPlatform::IndexedPack& mod_pack, ModPlatform::IndexedVersion& mod_version) -> Mod
{
    Mod mod;

    mod.name = mod_pack.name;
    mod.filename = mod_version.fileName;

    mod.url = mod_version.downloadUrl;
    mod.hash_format = ProviderCaps.hashType(mod_pack.provider);
    mod.hash = mod_version.hash;

    mod.provider = mod_pack.provider;
    mod.file_id = mod_pack.addonId;
    mod.project_id = mod_version.fileId;

    return mod;
}

auto V1::createModFormat(QDir& index_dir, ::Mod& internal_mod) -> Mod
{
    auto mod_name = internal_mod.name();

    // Try getting metadata if it exists
    Mod mod { getIndexForMod(index_dir, mod_name) };
    if(mod.isValid())
        return mod;

    qWarning() << QString("Tried to create mod metadata with a Mod without metadata!");

    return {};
}

void V1::updateModIndex(QDir& index_dir, Mod& mod)
{
    if(!mod.isValid()){
        qCritical() << QString("Tried to update metadata of an invalid mod!");
        return;
    }

    // Ensure the corresponding mod's info exists, and create it if not
    QFile index_file(index_dir.absoluteFilePath(indexFileName(mod.name)));

    // There's already data on there!
    // TODO: We should do more stuff here, as the user is likely trying to
    // override a file. In this case, check versions and ask the user what
    // they want to do!
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
        in_stream << QString("[update.%1]\n").arg(ProviderCaps.name(mod.provider));
        switch(mod.provider){
        case(ModPlatform::Provider::FLAME):
            in_stream << QString("file-id = %1\n").arg(mod.file_id.toString());
            in_stream << QString("project-id = %1\n").arg(mod.project_id.toString());
            break;
        case(ModPlatform::Provider::MODRINTH):
            addToStream("mod-id", mod.mod_id().toString());
            addToStream("version", mod.version().toString());
            break;
        }
    }
}

void V1::deleteModIndex(QDir& index_dir, QString& mod_name)
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

// Helper functions for extracting data from the TOML file
static auto stringEntry(toml_table_t* parent, const char* entry_name) -> QString
{
    toml_datum_t var = toml_string_in(parent, entry_name);
    if (!var.ok) {
        qCritical() << QString("Failed to read str property '%1' in mod metadata.").arg(entry_name);
        return {};
    }

    QString tmp = var.u.s;
    free(var.u.s);

    return tmp;
}

static auto intEntry(toml_table_t* parent, const char* entry_name) -> int
{
    toml_datum_t var = toml_int_in(parent, entry_name);
    if (!var.ok) {
        qCritical() << QString("Failed to read int property '%1' in mod metadata.").arg(entry_name);
        return {};
    }

    return var.u.i;
}

auto V1::getIndexForMod(QDir& index_dir, QString& index_file_name) -> Mod
{
    Mod mod;

    QFile index_file(index_dir.absoluteFilePath(indexFileName(index_file_name)));

    if (!index_file.exists()) {
        qWarning() << QString("Tried to get a non-existent mod metadata for %1").arg(index_file_name);
        return {};
    }
    if (!index_file.open(QIODevice::ReadOnly)) {
        qWarning() << QString("Failed to open mod metadata for %1").arg(index_file_name);
        return {};
    }

    toml_table_t* table = nullptr;

    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    char errbuf[200];
    table = toml_parse(index_file.readAll().data(), errbuf, sizeof(errbuf));

    index_file.close();

    if (!table) {
        qCritical() << QString("Could not open file %1!").arg(indexFileName(mod.name));
        return {};
    }
    
    { // Basic info
        mod.name = stringEntry(table, "name");
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
        using Provider = ModPlatform::Provider;

        toml_table_t* update_table = toml_table_in(table, "update");
        if (!update_table) {
            qCritical() << QString("No [update] section found on mod metadata!");
            return {};
        }

        toml_table_t* mod_provider_table = nullptr;
        if ((mod_provider_table = toml_table_in(update_table, ProviderCaps.name(Provider::FLAME)))) {
            mod.provider = Provider::FLAME;
            mod.file_id = intEntry(mod_provider_table, "file-id");
            mod.project_id = intEntry(mod_provider_table, "project-id");
        } else if ((mod_provider_table = toml_table_in(update_table, ProviderCaps.name(Provider::MODRINTH)))) {
            mod.provider = Provider::MODRINTH;
            mod.mod_id() = stringEntry(mod_provider_table, "mod-id");
            mod.version() = stringEntry(mod_provider_table, "version");
        } else {
            qCritical() << QString("No mod provider on mod metadata!");
            return {};
        }
        
    }

    toml_free(table);

    return mod;
}

} // namespace Packwiz
