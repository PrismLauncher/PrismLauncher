#include "ModFolderLoadTask.h"
#include <QDebug>

#include "Application.h"
#include "minecraft/mod/MetadataHandler.h"

ModFolderLoadTask::ModFolderLoadTask(QDir& mods_dir, QDir& index_dir) 
    : m_mods_dir(mods_dir), m_index_dir(index_dir), m_result(new Result())
{}

void ModFolderLoadTask::run()
{
    if (!APPLICATION->settings()->get("ModMetadataDisabled").toBool()) {
        // Read metadata first
        getFromMetadata();
    }

    // Read JAR files that don't have metadata
    m_mods_dir.refresh();
    for (auto entry : m_mods_dir.entryInfoList()) {
        Mod mod(entry);
        if(m_result->mods.contains(mod.internal_id())){
            m_result->mods[mod.internal_id()].setStatus(ModStatus::Installed);
        }
        else {
            m_result->mods[mod.internal_id()] = mod;
            m_result->mods[mod.internal_id()].setStatus(ModStatus::NoMetadata);
        }
    }

    emit succeeded();
}

void ModFolderLoadTask::getFromMetadata()
{
    m_index_dir.refresh();
    for (auto entry : m_index_dir.entryList(QDir::Files)) {
        auto metadata = Metadata::get(m_index_dir, entry);

        if(!metadata.isValid()){
            return;
        }

        Mod mod(m_mods_dir, metadata);
        mod.setStatus(ModStatus::NotInstalled);
        m_result->mods[mod.internal_id()] = mod;
    }
}
