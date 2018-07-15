#include "CustomCommands.h"
#include "ui_CustomCommands.h"

CustomCommands::~CustomCommands()
{
}

CustomCommands::CustomCommands(QWidget* parent):
    QWidget(parent),
    ui(new Ui::CustomCommands)
{
    ui->setupUi(this);
}

void CustomCommands::initialize(bool checkable, bool checked, const QString& prelaunch, const QString& wrapper, const QString& postexit)
{
    ui->customCommandsGroupBox->setCheckable(checkable);
    if(checkable)
    {
        ui->customCommandsGroupBox->setChecked(checked);
    }
    ui->preLaunchCmdTextBox->setText(prelaunch);
    ui->wrapperCmdTextBox->setText(wrapper);
    ui->postExitCmdTextBox->setText(postexit);
}


bool CustomCommands::checked() const
{
    if(!ui->customCommandsGroupBox->isCheckable())
        return true;
    return ui->customCommandsGroupBox->isChecked();
}

QString CustomCommands::prelaunchCommand() const
{
    return ui->preLaunchCmdTextBox->text();
}

QString CustomCommands::wrapperCommand() const
{
    return ui->wrapperCmdTextBox->text();
}

QString CustomCommands::postexitCommand() const
{
    return ui->postExitCmdTextBox->text();
}
