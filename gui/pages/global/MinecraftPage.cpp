/* Copyright 2013-2014 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "MinecraftPage.h"
#include "ui_MinecraftPage.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDir>

#include <pathutils.h>

#include "gui/Platform.h"
#include "gui/dialogs/VersionSelectDialog.h"
#include "gui/dialogs/CustomMessageBox.h"

#include "logic/NagUtils.h"

#include "logic/java/JavaUtils.h"
#include "logic/java/JavaVersionList.h"
#include "logic/java/JavaChecker.h"

#include "logic/updater/UpdateChecker.h"

#include "logic/tools/BaseProfiler.h"

#include "logic/settings/SettingsObject.h"
#include "MultiMC.h"

MinecraftPage::MinecraftPage(QWidget *parent) : QWidget(parent), ui(new Ui::MinecraftPage)
{
	ui->setupUi(this);
	ui->tabWidget->tabBar()->hide();
	loadSettings();
	updateCheckboxStuff();
}

MinecraftPage::~MinecraftPage()
{
	delete ui;
}

bool MinecraftPage::apply()
{
	applySettings();
	return true;
}

void MinecraftPage::updateCheckboxStuff()
{
	ui->windowWidthSpinBox->setEnabled(!ui->maximizedCheckBox->isChecked());
	ui->windowHeightSpinBox->setEnabled(!ui->maximizedCheckBox->isChecked());
}

void MinecraftPage::on_maximizedCheckBox_clicked(bool checked)
{
	Q_UNUSED(checked);
	updateCheckboxStuff();
}


void MinecraftPage::applySettings()
{
	auto s = MMC->settings();
	// Minecraft version updates
	s->set("AutoUpdateMinecraftVersions", ui->autoupdateMinecraft->isChecked());

	// Window Size
	s->set("LaunchMaximized", ui->maximizedCheckBox->isChecked());
	s->set("MinecraftWinWidth", ui->windowWidthSpinBox->value());
	s->set("MinecraftWinHeight", ui->windowHeightSpinBox->value());
}

void MinecraftPage::loadSettings()
{
	auto s = MMC->settings();
	// Minecraft version updates
	ui->autoupdateMinecraft->setChecked(s->get("AutoUpdateMinecraftVersions").toBool());

	// Window Size
	ui->maximizedCheckBox->setChecked(s->get("LaunchMaximized").toBool());
	ui->windowWidthSpinBox->setValue(s->get("MinecraftWinWidth").toInt());
	ui->windowHeightSpinBox->setValue(s->get("MinecraftWinHeight").toInt());
}
