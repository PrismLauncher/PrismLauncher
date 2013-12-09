#include "UpdateDialog.h"
#include "ui_UpdateDialog.h"
#include "gui/Platform.h"

UpdateDialog::UpdateDialog(QWidget *parent) : QDialog(parent), ui(new Ui::UpdateDialog)
{
	MultiMCPlatform::fixWM_CLASS(this);
	ui->setupUi(this);
}

UpdateDialog::~UpdateDialog()
{
}

void UpdateDialog::on_btnUpdateLater_clicked()
{
	reject();
}

void UpdateDialog::on_btnUpdateNow_clicked()
{
	done(UPDATE_NOW);
}

void UpdateDialog::on_btnUpdateOnExit_clicked()
{
	done(UPDATE_ONEXIT);
}
