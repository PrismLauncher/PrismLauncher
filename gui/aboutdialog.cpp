#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include <QIcon>
#include <QApplication>

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    ui->icon->setPixmap(QIcon(":/icons/multimc/scalable/apps/multimc.svg").pixmap(64));

    connect(ui->closeButton, SIGNAL(clicked()), SLOT(close()));

    QApplication::instance()->connect(ui->aboutQt, SIGNAL(clicked()), SLOT(aboutQt()));
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
