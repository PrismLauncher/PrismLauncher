#include "RecursiveFileSystemWatcher.h"

#include <QRegularExpression>
#include <QDebug>

RecursiveFileSystemWatcher::RecursiveFileSystemWatcher(QObject *parent)
    : QObject(parent), m_watcher(new QFileSystemWatcher(this))
{
    connect(m_watcher, &QFileSystemWatcher::fileChanged, this,
            &RecursiveFileSystemWatcher::fileChange);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this,
            &RecursiveFileSystemWatcher::directoryChange);
}

void RecursiveFileSystemWatcher::setRootDir(const QDir &root)
{
    bool wasEnabled = m_isEnabled;
    disable();
    m_root = root;
    setFiles(scanRecursive(m_root));
    if (wasEnabled)
    {
        enable();
    }
}
void RecursiveFileSystemWatcher::setWatchFiles(const bool watchFiles)
{
    bool wasEnabled = m_isEnabled;
    disable();
    m_watchFiles = watchFiles;
    if (wasEnabled)
    {
        enable();
    }
}

void RecursiveFileSystemWatcher::enable()
{
    if (m_isEnabled)
    {
        return;
    }
    Q_ASSERT(m_root != QDir::root());
    addFilesToWatcherRecursive(m_root);
    m_isEnabled = true;
}
void RecursiveFileSystemWatcher::disable()
{
    if (!m_isEnabled)
    {
        return;
    }
    m_isEnabled = false;
    m_watcher->removePaths(m_watcher->files());
    m_watcher->removePaths(m_watcher->directories());
}

void RecursiveFileSystemWatcher::setFiles(const QStringList &files)
{
    if (files != m_files)
    {
        m_files = files;
        emit filesChanged();
    }
}

void RecursiveFileSystemWatcher::addFilesToWatcherRecursive(const QDir &dir)
{
    m_watcher->addPath(dir.absolutePath());
    for (const QString &directory : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
    {
        addFilesToWatcherRecursive(dir.absoluteFilePath(directory));
    }
    if (m_watchFiles)
    {
        for (const QFileInfo &info : dir.entryInfoList(QDir::Files))
        {
            m_watcher->addPath(info.absoluteFilePath());
        }
    }
}
QStringList RecursiveFileSystemWatcher::scanRecursive(const QDir &directory)
{
    QStringList ret;
    if(!m_matcher)
    {
        return {};
    }
    for (const QString &dir : directory.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden))
    {
        ret.append(scanRecursive(directory.absoluteFilePath(dir)));
    }
    for (const QString &file : directory.entryList(QDir::Files | QDir::Hidden))
    {
        auto relPath = m_root.relativeFilePath(directory.absoluteFilePath(file));
        if (m_matcher->matches(relPath))
        {
            ret.append(relPath);
        }
    }
    return ret;
}

void RecursiveFileSystemWatcher::fileChange(const QString &path)
{
    emit fileChanged(path);
}
void RecursiveFileSystemWatcher::directoryChange(const QString &path)
{
    setFiles(scanRecursive(m_root));
}
