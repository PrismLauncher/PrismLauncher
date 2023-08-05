
#pragma once

#include <QFileSystemWatcher>
#include <QString>

struct WatchLock {
    WatchLock(QFileSystemWatcher* watcher, const QString& directory) : m_watcher(watcher), m_directory(directory)
    {
        m_watcher->removePath(m_directory);
    }
    ~WatchLock() { m_watcher->addPath(m_directory); }
    QFileSystemWatcher* m_watcher;
    QString m_directory;
};
