#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include <QIcon>
#include <MultiMC.h>
#include "gui/platform.h"

AboutDialog::AboutDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::AboutDialog)
{
    MultiMCPlatform::fixWM_CLASS(this);
	ui->setupUi(this);

	ui->icon->setPixmap(QIcon(":/icons/multimc/scalable/apps/multimc.svg").pixmap(64));
	ui->title->setText("MultiMC " + MMC->version().toString());
	connect(ui->closeButton, SIGNAL(clicked()), SLOT(close()));

	MMC->connect(ui->aboutQt, SIGNAL(clicked()), SLOT(aboutQt()));
}

AboutDialog::~AboutDialog()
{
	delete ui;
}
