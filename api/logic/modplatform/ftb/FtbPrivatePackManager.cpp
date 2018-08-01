#include "FtbPrivatePackManager.h"

#include <QDebug>

#include "FileSystem.h"

void FtbPrivatePackManager::load()
{
    try
    {
        currentPacks = QString::fromUtf8(FS::read(m_filename)).split('\n', QString::SkipEmptyParts).toSet();
        dirty = false;
    }
    catch(...)
    {
        currentPacks = {};
        qWarning() << "Failed to read third party FTB pack codes from" << m_filename;
    }
}

void FtbPrivatePackManager::save() const
{
    if(!dirty)
    {
        return;
    }
    try
    {
        QStringList list = currentPacks.toList();
        FS::write(m_filename, list.join('\n').toUtf8());
        dirty = false;
    }
    catch(...)
    {
        qWarning() << "Failed to write third party FTB pack codes to" << m_filename;
    }
}
