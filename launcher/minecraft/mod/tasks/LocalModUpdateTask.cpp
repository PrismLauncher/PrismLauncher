#include "LocalModUpdateTask.h"

#include <toml.h>

#include "FileSystem.h"
#include "modplatform/packwiz/Packwiz.h"

LocalModUpdateTask::LocalModUpdateTask(QDir index_dir, ModPlatform::IndexedPack& mod, ModPlatform::IndexedVersion& mod_version)
    : m_index_dir(index_dir), m_mod(mod), m_mod_version(mod_version)
{
    // Ensure a '.index' folder exists in the mods folder, and create it if it does not
    if (!FS::ensureFolderPathExists(index_dir.path())) {
        emitFailed(QString("Unable to create index for mod %1!").arg(m_mod.name));
    }
}

void LocalModUpdateTask::executeTask()
{
    setStatus(tr("Updating index for mod:\n%1").arg(m_mod.name));

    auto pw_mod = Packwiz::createModFormat(m_index_dir, m_mod, m_mod_version);
    Packwiz::updateModIndex(m_index_dir, pw_mod);

    emitSucceeded();
}

bool LocalModUpdateTask::abort()
{
    emitAborted();
    return true;
}
