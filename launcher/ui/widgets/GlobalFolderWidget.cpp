#include "GlobalFolderWidget.h"
#include "ui_GlobalFolderWidget.h"

#include <QFileDialog>

#include "FileSystem.h"

GlobalFolderWidget::GlobalFolderWidget(QWidget* parent) : QWidget(parent), ui(new Ui::GlobalFolderWidget)
{
    ui->setupUi(this);
}

GlobalFolderWidget::~GlobalFolderWidget()
{
    delete ui;
}

void GlobalFolderWidget::initialize(bool enabled, const QString& path, const QString& label)
{
    ui->enabledCheckBox->setChecked(enabled);
    ui->enabledCheckBox->setText(label);

    ui->pathTextBox->setEnabled(enabled);
    ui->pathTextBox->setText(path);

    ui->pathBrowseBtn->setEnabled(enabled);
}

bool GlobalFolderWidget::isEnabled() const
{
    return ui->enabledCheckBox->isChecked();
}

QString GlobalFolderWidget::getPath() const
{
    return ui->pathTextBox->text();
}

void GlobalFolderWidget::on_enabledCheckBox_toggled(bool checked)
{
    ui->pathTextBox->setEnabled(checked);
    ui->pathBrowseBtn->setEnabled(checked);
}

void GlobalFolderWidget::on_pathBrowseBtn_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Select global folder"), ui->pathTextBox->text());
    if (path.isEmpty()) {
        return;
    }

    ui->pathTextBox->setText(path);
}
