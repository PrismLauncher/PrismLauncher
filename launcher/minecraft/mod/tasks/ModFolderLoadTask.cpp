#include "ModFolderLoadTask.h"
#include <QDebug>

#include "Application.h"
#include "minecraft/mod/MetadataHandler.h"

ModFolderLoadTask::ModFolderLoadTask(QDir& mods_dir, QDir& index_dir) 
    : m_mods_dir(mods_dir), m_index_dir(index_dir), m_result(new Result())
{}

void ModFolderLoadTask::run()
{
    if (!APPLICATION->settings()->get("DontUseModMetadata").toBool()) {
        // Read metadata first
        getFromMetadata();
    }

    // Read JAR files that don't have metadata
    m_mods_dir.refresh();
    for (auto entry : m_mods_dir.entryInfoList()) {
        Mod mod(entry);
        if (!m_result->mods.contains(mod.internal_id()))
            m_result->mods[mod.internal_id()] = mod;
    }

    emit succeeded();
}

void ModFolderLoadTask::getFromMetadata()
{
    m_index_dir.refresh();
    for (auto entry : m_index_dir.entryList()) {
        // QDir::Filter::NoDotAndDotDot seems to exclude all files for some reason...
        if (entry == "." || entry == "..")
            continue;

        entry.chop(5);  // Remove .toml at the end
        Mod mod(m_mods_dir, Metadata::get(m_index_dir, entry));
        m_result->mods[mod.internal_id()] = mod;
    }
}
