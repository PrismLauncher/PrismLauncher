
#pragma once

#include <QString>
#include <QFileSystemWatcher>

struct WatchLock
{
    WatchLock(QFileSystemWatcher * watcher, const QString& directory)
        : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_watcher(watcher), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_directory(directory)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_watcher->removePath(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_directory);
    }
    ~WatchLock()
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_watcher->addPath(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_directory);
    }
    QFileSystemWatcher * hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_watcher;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_directory;
};
