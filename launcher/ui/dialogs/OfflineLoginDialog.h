#pragma once

#include <QtCore/QEventLoop>
#include <QtWidgets/QDialog>

#include "minecraft/auth/MinecraftAccount.h"
#include "tasks/Task.h"

namespace Ui {
class OfflineLoginDialog;
}

class OfflineLoginDialog : public QDialog {
    Q_OBJECT

   public:
    ~OfflineLoginDialog();

    static MinecraftAccountPtr newAccount(QWidget* parent, QString message);

   private:
    explicit OfflineLoginDialog(QWidget* parent = 0);

    void setUserInputsEnabled(bool enable);

   protected slots:
    void accept();

    void onTaskFinished(TaskV2* t);
    void onTaskStatus(TaskV2* t);
    void onTaskProgress(TaskV2* t, double current, double delta);
    void onTaskProgressTotal(TaskV2* t, double total, double delta);

    void on_userTextBox_textEdited(const QString& newText);
    void on_allowLongUsernames_stateChanged(int value);

   private:
    Ui::OfflineLoginDialog* ui;
    MinecraftAccountPtr m_account;
    TaskV2::Ptr m_loginTask;
};
