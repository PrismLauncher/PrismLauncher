#pragma once

#include <QDialog>
#include <minecraft/auth/MinecraftAccount.h>

namespace Ui
{
    class SkinUploadDialog;
}

class SkinUploadDialog : public QDialog {
    Q_OBJECT
public:
    explicit SkinUploadDialog(MinecraftAccountPtr acct, QWidget *parent = 0);
    virtual ~SkinUploadDialog() {};

public slots:
    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

    void on_skinBrowseBtn_clicked();

protected:
    MinecraftAccountPtr hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_acct;

private:
    Ui::SkinUploadDialog *ui;
};
