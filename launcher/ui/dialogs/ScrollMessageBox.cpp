#include "ScrollMessageBox.h"
#include "ui_ScrollMessageBox.h"

ScrollMessageBox::ScrollMessageBox(QWidget* parent, const QString& title, const QString& text, const QString& body)
    : QDialog(parent), ui(new Ui::ScrollMessageBox)
{
    ui->setupUi(this);
    this->setWindowTitle(title);
    ui->label->setText(text);
    ui->textBrowser->setText(body);
}

ScrollMessageBox::~ScrollMessageBox()
{
    delete ui;
}
