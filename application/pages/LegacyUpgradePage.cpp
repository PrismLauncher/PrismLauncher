#include "LegacyUpgradePage.h"
#include "ui_LegacyUpgradePage.h"

#include "minecraft/legacy/LegacyInstance.h"
#include "minecraft/legacy/LegacyUpgradeTask.h"
#include "MultiMC.h"
#include "FolderInstanceProvider.h"
#include "dialogs/CustomMessageBox.h"
#include "dialogs/ProgressDialog.h"

LegacyUpgradePage::LegacyUpgradePage(InstancePtr inst, QWidget *parent)
	: QWidget(parent), ui(new Ui::LegacyUpgradePage), m_inst(inst)
{
	ui->setupUi(this);
}

LegacyUpgradePage::~LegacyUpgradePage()
{
	delete ui;
}

void LegacyUpgradePage::runModalTask(Task *task)
{
	connect(task, &Task::failed, [this](QString reason)
		{
			CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Warning)->show();
		});
	ProgressDialog loadDialog(this);
	loadDialog.setSkipButton(true, tr("Abort"));
	if(loadDialog.execWithTask(task) == QDialog::Accepted)
	{
		m_container->requestClose();
	}
}

void LegacyUpgradePage::on_upgradeButton_clicked()
{
	std::unique_ptr<Task> task(MMC->folderProvider()->legacyUpgradeTask(m_inst));
	runModalTask(task.get());
}

bool LegacyUpgradePage::shouldDisplay() const
{
	return !m_inst->isRunning();
}
