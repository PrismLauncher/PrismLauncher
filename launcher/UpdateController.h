#pragma once

#include <QString>
#include <QList>
#include <updater/GoUpdate.h>

class QWidget;

class UpdateController
{
public:
    UpdateController(QWidget * parent, const QString &root, const QString updateFilesDir, GoUpdate::OperationList operations);
    void installUpdates();

private:
    void fail();
    bool rollback();

private:
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_root;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateFilesDir;
    GoUpdate::OperationList hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_operations;
    QWidget * hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent;

    struct BackupEntry
    {
        // path where we got the new file from
        QString update;
        // path of what is being actually updated
        QString original;
        // path where the backup of the updated file was placed
        QString backup;
    };
    QList <BackupEntry> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_replace_backups;
    QList <BackupEntry> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_delete_backups;
    enum Failure
    {
        Replace,
        Delete,
        Start,
        Nothing
    } hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_failedOperationType = Nothing;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_failedFile;
};
