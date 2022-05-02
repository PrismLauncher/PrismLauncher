#include <QFileInfo>
#include <QFileDialog>
#include <QPainter>

#include <FileSystem.h>

#include <minecraft/services/SkinUpload.h>
#include <minecraft/services/CapeChange.h>
#include <tasks/SequentialTask.h>

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
    QString fileName;
    QString input = ui->skinPathTextBox->text();
    QRegularExpression urlPrefixMatcher(QRegularExpression::anchoredPattern("^([a-z]+)://.+$"));
    bool isLocalFile = false;
    // it has an URL prefix -> it is an URL
    if(urlPrefixMatcher.match(input).hasMatch())  // TODO: does this work?
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
    ProgressDialog prog(this);
    SequentialTask skinUpload;
    skinUpload.addTask(shared_qobject_ptr<SkinUpload>(new SkinUpload(this, m_acct->accessToken(), FS::read(fileName), model)));
    auto selectedCape = ui->capeCombo->currentData().toString();
    if(selectedCape != m_acct->accountData()->minecraftProfile.currentCape) {
        skinUpload.addTask(shared_qobject_ptr<CapeChange>(new CapeChange(this, m_acct->accessToken(), selectedCape)));
    }
    if (prog.execWithTask(&skinUpload) != QDialog::Accepted)
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
    auto filter = QMimeDatabase().mimeTypeForName("image/png").filterString();
    QString raw_path = QFileDialog::getOpenFileName(this, tr("Select Skin Texture"), QString(), filter);
    if (raw_path.isEmpty() || !QFileInfo::exists(raw_path))
    {
        return;
    }
    QString cooked_path = FS::NormalizePath(raw_path);
    ui->skinPathTextBox->setText(cooked_path);
}

SkinUploadDialog::SkinUploadDialog(MinecraftAccountPtr acct, QWidget *parent)
    :QDialog(parent), m_acct(acct), ui(new Ui::SkinUploadDialog)
{
    ui->setupUi(this);

    // FIXME: add a model for this, download/refresh the capes on demand
    auto &data = *acct->accountData();
    int index = 0;
    ui->capeCombo->addItem(tr("No Cape"), QVariant());
    auto currentCape = data.minecraftProfile.currentCape;
    if(currentCape.isEmpty()) {
        ui->capeCombo->setCurrentIndex(index);
    }

    for(auto & cape: data.minecraftProfile.capes) {
        index++;
        if(cape.data.size()) {
            QPixmap capeImage;
            if(capeImage.loadFromData(cape.data, "PNG")) {
                QPixmap preview = QPixmap(10, 16);
                QPainter painter(&preview);
                painter.drawPixmap(0, 0, capeImage.copy(1, 1, 10, 16));
                ui->capeCombo->addItem(capeImage, cape.alias, cape.id);
                if(currentCape == cape.id) {
                    ui->capeCombo->setCurrentIndex(index);
                }
                continue;
            }
        }
        ui->capeCombo->addItem(cape.alias, cape.id);
        if(currentCape == cape.id) {
            ui->capeCombo->setCurrentIndex(index);
        }
    }
}
