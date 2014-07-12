#include "LegacyUpgradePage.h"
#include <logic/LegacyInstance.h>
#include "ui_LegacyUpgradePage.h"

QString LegacyUpgradePage::displayName() const
{
	return tr("Upgrade");
}

QIcon LegacyUpgradePage::icon() const
{
	return QIcon::fromTheme("checkupdate");
}

QString LegacyUpgradePage::id() const
{
	return "upgrade";
}

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
