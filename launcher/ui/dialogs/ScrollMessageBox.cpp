#include "ScrollMessageBox.h"
#include <QPushButton>
#include "ui_ScrollMessageBox.h"

ScrollMessageBox::ScrollMessageBox(QWidget* parent, const QString& title, const QString& text, const QString& body)
    : QDialog(parent), ui(new Ui::ScrollMessageBox)
{
    ui->setupUi(this);
    this->setWindowTitle(title);
    ui->label->setText(text);
    ui->textBrowser->setText(body);

    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
}

ScrollMessageBox::~ScrollMessageBox()
{
    delete ui;
}
