#include "PrivatePackManager.h"

#include <QDebug>

#include "FileSystem.h"

namespace LegacyFTB {

void PrivatePackManager::load()
{
    try
    {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        auto foo = QString::fromUtf8(FS::read(m_filename)).split('\n', Qt::SkipEmptyParts);
        currentPacks = QSet<QString>(foo.begin(), foo.end());
#else
        currentPacks = QString::fromUtf8(FS::read(m_filename)).split('\n', QString::SkipEmptyParts).toSet();
#endif

        dirty = false;
    }
    catch(...)
    {
        currentPacks = {};
        qWarning() << "Failed to read third party FTB pack codes from" << m_filename;
    }
}

void PrivatePackManager::save() const
{
    if(!dirty)
    {
        return;
    }
    try
    {
        QStringList list = currentPacks.values();
        FS::write(m_filename, list.join('\n').toUtf8());
        dirty = false;
    }
    catch(...)
    {
        qWarning() << "Failed to write third party FTB pack codes to" << m_filename;
    }
}

}
