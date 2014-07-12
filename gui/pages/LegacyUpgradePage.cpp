#include "LegacyUpgradePage.h"
#include "ui_LegacyUpgradePage.h"

#include "logic/LegacyInstance.h"

LegacyUpgradePage::LegacyUpgradePage(LegacyInstance *inst, QWidget *parent)
	: QWidget(parent), ui(new Ui::LegacyUpgradePage), m_inst(inst)
{
	ui->setupUi(this);
}

LegacyUpgradePage::~LegacyUpgradePage()
{
	delete ui;
}

void LegacyUpgradePage::on_upgradeButton_clicked()
{
	// now what?
}

bool LegacyUpgradePage::shouldDisplay() const
{
	return !m_inst->isRunning();
}
