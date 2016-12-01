#include "SetupWizard.h"

enum Page
{
	Language,
	Java,
	Analytics,
	Themes,
	Accounts
};
#include "ui_SetupWizard.h"


SetupWizard::SetupWizard(QWidget *parent):QWizard(parent), ui(new Ui::SetupWizard)
{
	ui->setupUi(this);
}

SetupWizard::~SetupWizard()
{
}

bool SetupWizard::isRequired()
{
	return true;
}
