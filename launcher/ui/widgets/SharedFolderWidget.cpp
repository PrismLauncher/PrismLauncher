#include "SharedFolderWidget.h"
#include "ui_SharedFolderWidget.h"

#include <QFileDialog>

#include "FileSystem.h"

SharedFolderWidget::SharedFolderWidget(QWidget* parent) : QWidget(parent), ui(new Ui::SharedFolderWidget)
{
    ui->setupUi(this);
}

SharedFolderWidget::~SharedFolderWidget()
{
    delete ui;
}

void SharedFolderWidget::initialize(bool enabled, const QString& path, const QString& label)
{
    ui->enabledCheckBox->setChecked(enabled);
    ui->enabledCheckBox->setText(label);

    ui->pathTextBox->setEnabled(enabled);
    ui->pathTextBox->setText(path);

    ui->pathBrowseBtn->setEnabled(enabled);
}

bool SharedFolderWidget::isEnabled() const
{
    return ui->enabledCheckBox->isChecked();
}

QString SharedFolderWidget::getPath() const
{
    return ui->pathTextBox->text();
}

void SharedFolderWidget::on_enabledCheckBox_toggled(bool checked)
{
    ui->pathTextBox->setEnabled(checked);
    ui->pathBrowseBtn->setEnabled(checked);
}

void SharedFolderWidget::on_pathBrowseBtn_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Select shared folder"), ui->pathTextBox->text());
    if (path.isEmpty()) {
        return;
    }

    ui->pathTextBox->setText(path);
}
