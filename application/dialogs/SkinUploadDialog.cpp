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
		CustomMessageBox::selectable(this, tr("Failed to login!"), tr("Unknown error"), QMessageBox::Warning)->exec();
		close();
		return;
	}
	QString fileName = ui->skinPathTextBox->text();
	if (!QFile::exists(fileName))
	{
		CustomMessageBox::selectable(this, tr("Skin file does not exist!"), tr("Unknown error"), QMessageBox::Warning)->exec();
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
		CustomMessageBox::selectable(this, tr("Failed to upload skin!"), tr("Unknown error"), QMessageBox::Warning)->exec();
		close();
		return;
	}
	CustomMessageBox::selectable(this, tr("Skin uploaded!"), tr("Success"), QMessageBox::Information)->exec();
	close();
}

void SkinUploadDialog::on_skinBrowseBtn_clicked()
{
	QString raw_path = QFileDialog::getOpenFileName(this, tr("Select Skin Texture"), QString(), "*.png");
	QString cooked_path = FS::NormalizePath(raw_path);
	if (cooked_path.isEmpty() || !QFileInfo::exists(cooked_path))
	{
		return;
	}
	ui->skinPathTextBox->setText(cooked_path);
}

SkinUploadDialog::SkinUploadDialog(MojangAccountPtr acct, QWidget *parent)
	:QDialog(parent), m_acct(acct), ui(new Ui::SkinUploadDialog)
{
	ui->setupUi(this);
}
