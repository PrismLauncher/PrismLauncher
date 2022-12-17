#pragma once

#include <QtWidgets/QDialog>
#include <QtCore/QEventLoop>

#include "minecraft/auth/MinecraftAccount.h"
#include "tasks/Task.h"

namespace Ui
{
class OfflineLoginDialog;
}

class OfflineLoginDialog : public QDialog
{
    Q_OBJECT

public:
    ~OfflineLoginDialog();

    static MinecraftAccountPtr newAccount(QWidget *parent, QString message);

private:
    explicit OfflineLoginDialog(QWidget *parent = 0);

    void setUserInputsEnabled(bool enable);

protected
slots:
    void accept();

    void onTaskFailed(const QString &reason);
    void onTaskSucceeded();
    void onTaskStatus(const QString &status);
    void onTaskProgress(qint64 current, qint64 total);

    void on_userTextBox_textEdited(const QString &newText);
    void on_allowLongUsernames_stateChanged(int value);

private:
    Ui::OfflineLoginDialog *ui;
    MinecraftAccountPtr hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_account;
    Task::Ptr hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_loginTask;
};
