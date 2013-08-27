#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include <AppVersion.h>

#include <QIcon>
#include <QApplication>

AboutDialog::AboutDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::AboutDialog)
{
	ui->setupUi(this);

	ui->icon->setPixmap(QIcon(":/icons/multimc/scalable/apps/multimc.svg").pixmap(64));
	ui->title->setText("MultiMC " + AppVersion::current.toString());
	connect(ui->closeButton, SIGNAL(clicked()), SLOT(close()));

	QApplication::instance()->connect(ui->aboutQt, SIGNAL(clicked()), SLOT(aboutQt()));
}

AboutDialog::~AboutDialog()
{
	delete ui;
}
