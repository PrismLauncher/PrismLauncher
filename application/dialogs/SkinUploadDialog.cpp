#include <QFileInfo>
#include <QFileDialog>
#include <FileSystem.h>
#include <minecraft/SkinUpload.h>
#include "SkinUploadDialog.h"
#include "ui_SkinUploadDialog.h"
#include "ProgressDialog.h"
#include "CustomMessageBox.h"

void SkinUploadDialog::on_buttonBox_rejected()
{
    close();
}

void SkinUploadDialog::on_buttonBox_accepted()
{
    AuthSessionPtr session = std::make_shared<AuthSession>();
    auto login = m_acct->login(session);
    ProgressDialog prog(this);
    if (prog.execWithTask((Task*)login.get()) != QDialog::Accepted)
    {
        //FIXME: recover with password prompt
        CustomMessageBox::selectable(this, tr("Skin Upload"), tr("Failed to login!"), QMessageBox::Warning)->exec();
        close();
        return;
    }
    QString fileName;
    QString input = ui->skinPathTextBox->text();
    QRegExp urlPrefixMatcher("^([a-z]+)://.+$");
    bool isLocalFile = false;
    // it has an URL prefix -> it is an URL
    if(urlPrefixMatcher.exactMatch(input))
    {
        QUrl fileURL = input;
        if(fileURL.isValid())
        {
            // local?
            if(fileURL.isLocalFile())
            {
                isLocalFile = true;
                fileName = fileURL.toLocalFile();
            }
            else
            {
                CustomMessageBox::selectable(
                    this,
                    tr("Skin Upload"),
                    tr("Using remote URLs for setting skins is not implemented yet."),
                    QMessageBox::Warning
                )->exec();
                close();
                return;
            }
        }
        else
        {
            CustomMessageBox::selectable(
                this,
                tr("Skin Upload"),
                tr("You cannot use an invalid URL for uploading skins."),
                QMessageBox::Warning
            )->exec();
            close();
            return;
        }
    }
    else
    {
        // just assume it's a path then
        isLocalFile = true;
        fileName = ui->skinPathTextBox->text();
    }
    if (isLocalFile && !QFile::exists(fileName))
    {
        CustomMessageBox::selectable(this, tr("Skin Upload"), tr("Skin file does not exist!"), QMessageBox::Warning)->exec();
        close();
        return;
    }
    SkinUpload::Model model = SkinUpload::STEVE;
    if (ui->steveBtn->isChecked())
    {
        model = SkinUpload::STEVE;
    }
    else if (ui->alexBtn->isChecked())
    {
        model = SkinUpload::ALEX;
    }
    SkinUploadPtr upload = std::make_shared<SkinUpload>(this, session, FS::read(fileName), model);
    if (prog.execWithTask((Task*)upload.get()) != QDialog::Accepted)
    {
        CustomMessageBox::selectable(this, tr("Skin Upload"), tr("Failed to upload skin!"), QMessageBox::Warning)->exec();
        close();
        return;
    }
    CustomMessageBox::selectable(this, tr("Skin Upload"), tr("Success"), QMessageBox::Information)->exec();
    close();
}

void SkinUploadDialog::on_skinBrowseBtn_clicked()
{
    QString raw_path = QFileDialog::getOpenFileName(this, tr("Select Skin Texture"), QString(), "*.png");
    if (raw_path.isEmpty() || !QFileInfo::exists(raw_path))
    {
        return;
    }
    QString cooked_path = FS::NormalizePath(raw_path);
    ui->skinPathTextBox->setText(cooked_path);
}

SkinUploadDialog::SkinUploadDialog(MojangAccountPtr acct, QWidget *parent)
    :QDialog(parent), m_acct(acct), ui(new Ui::SkinUploadDialog)
{
    ui->setupUi(this);
}
