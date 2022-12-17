#pragma once

#include <QFileSystemWatcher>
#include <QDir>
#include "pathmatcher/IPathMatcher.h"

class RecursiveFileSystemWatcher : public QObject
{
    Q_OBJECT
public:
    RecursiveFileSystemWatcher(QObject *parent);

    void setRootDir(const QDir &root);
    QDir rootDir() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_root;
    }

    // WARNING: setting this to true may be bad for performance
    void setWatchFiles(const bool watchFiles);
    bool watchFiles() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_watchFiles;
    }

    void setMatcher(IPathMatcher::Ptr matcher)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_matcher = matcher;
    }

    QStringList files() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_files;
    }

signals:
    void filesChanged();
    void fileChanged(const QString &path);

public slots:
    void enable();
    void disable();

private:
    QDir hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_root;
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_watchFiles = false;
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_isEnabled = false;
    IPathMatcher::Ptr hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_matcher;

    QFileSystemWatcher *hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_watcher;

    QStringList hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_files;
    void setFiles(const QStringList &files);

    void addFilesToWatcherRecursive(const QDir &dir);
    QStringList scanRecursive(const QDir &dir);

private slots:
    void fileChange(const QString &path);
    void directoryChange(const QString &path);
};
