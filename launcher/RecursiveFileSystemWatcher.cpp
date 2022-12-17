#include "RecursiveFileSystemWatcher.h"

#include <QRegularExpression>
#include <QDebug>

RecursiveFileSystemWatcher::RecursiveFileSystemWatcher(QObject *parent)
    : QObject(parent), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_watcher(new QFileSystemWatcher(this))
{
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_watcher, &QFileSystemWatcher::fileChanged, this,
            &RecursiveFileSystemWatcher::fileChange);
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_watcher, &QFileSystemWatcher::directoryChanged, this,
            &RecursiveFileSystemWatcher::directoryChange);
}

void RecursiveFileSystemWatcher::setRootDir(const QDir &root)
{
    bool wasEnabled = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_isEnabled;
    disable();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_root = root;
    setFiles(scanRecursive(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_root));
    if (wasEnabled)
    {
        enable();
    }
}
void RecursiveFileSystemWatcher::setWatchFiles(const bool watchFiles)
{
    bool wasEnabled = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_isEnabled;
    disable();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_watchFiles = watchFiles;
    if (wasEnabled)
    {
        enable();
    }
}

void RecursiveFileSystemWatcher::enable()
{
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_isEnabled)
    {
        return;
    }
    Q_ASSERT(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_root != QDir::root());
    addFilesToWatcherRecursive(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_root);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_isEnabled = true;
}
void RecursiveFileSystemWatcher::disable()
{
    if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_isEnabled)
    {
        return;
    }
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_isEnabled = false;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_watcher->removePaths(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_watcher->files());
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_watcher->removePaths(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_watcher->directories());
}

void RecursiveFileSystemWatcher::setFiles(const QStringList &files)
{
    if (files != hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_files)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_files = files;
        emit filesChanged();
    }
}

void RecursiveFileSystemWatcher::addFilesToWatcherRecursive(const QDir &dir)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_watcher->addPath(dir.absolutePath());
    for (const QString &directory : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
    {
        addFilesToWatcherRecursive(dir.absoluteFilePath(directory));
    }
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_watchFiles)
    {
        for (const QFileInfo &info : dir.entryInfoList(QDir::Files))
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_watcher->addPath(info.absoluteFilePath());
        }
    }
}
QStringList RecursiveFileSystemWatcher::scanRecursive(const QDir &directory)
{
    QStringList ret;
    if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_matcher)
    {
        return {};
    }
    for (const QString &dir : directory.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden))
    {
        ret.append(scanRecursive(directory.absoluteFilePath(dir)));
    }
    for (const QString &file : directory.entryList(QDir::Files | QDir::Hidden))
    {
        auto relPath = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_root.relativeFilePath(directory.absoluteFilePath(file));
        if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_matcher->matches(relPath))
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
    setFiles(scanRecursive(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_root));
}
