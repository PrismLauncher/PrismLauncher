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

#include "JavaPage.h"
#include "ui_JavaPage.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDir>

#include <pathutils.h>

#include "logic/NagUtils.h"

#include "gui/Platform.h"
#include "gui/dialogs/VersionSelectDialog.h"
#include <gui/ColumnResizer.h>

#include "logic/java/JavaUtils.h"
#include "logic/java/JavaVersionList.h"
#include "logic/java/JavaChecker.h"

#include "logic/settings/SettingsObject.h"
#include "MultiMC.h"

JavaPage::JavaPage(QWidget *parent) : QWidget(parent), ui(new Ui::JavaPage)
{
	ui->setupUi(this);
	ui->tabWidget->tabBar()->hide();
	
	auto resizer = new ColumnResizer(this);
	resizer->addWidgetsFromLayout(ui->javaSettingsGroupBox->layout(), 0);
	resizer->addWidgetsFromLayout(ui->customCommandsGroupBox->layout(), 0);
	
	loadSettings();
}

JavaPage::~JavaPage()
{
	delete ui;
}

bool JavaPage::apply()
{
	applySettings();
	return true;
}

void JavaPage::applySettings()
{
	auto s = MMC->settings();
	// Memory
	s->set("MinMemAlloc", ui->minMemSpinBox->value());
	s->set("MaxMemAlloc", ui->maxMemSpinBox->value());
	s->set("PermGen", ui->permGenSpinBox->value());

	// Java Settings
	s->set("JavaPath", ui->javaPathTextBox->text());
	s->set("JvmArgs", ui->jvmArgsTextBox->text());
	NagUtils::checkJVMArgs(s->get("JvmArgs").toString(), this->parentWidget());

	// Custom Commands
	s->set("PreLaunchCommand", ui->preLaunchCmdTextBox->text());
	s->set("PostExitCommand", ui->postExitCmdTextBox->text());
}
void JavaPage::loadSettings()
{
	auto s = MMC->settings();
	// Memory
	ui->minMemSpinBox->setValue(s->get("MinMemAlloc").toInt());
	ui->maxMemSpinBox->setValue(s->get("MaxMemAlloc").toInt());
	ui->permGenSpinBox->setValue(s->get("PermGen").toInt());

	// Java Settings
	ui->javaPathTextBox->setText(s->get("JavaPath").toString());
	ui->jvmArgsTextBox->setText(s->get("JvmArgs").toString());

	// Custom Commands
	ui->preLaunchCmdTextBox->setText(s->get("PreLaunchCommand").toString());
	ui->postExitCmdTextBox->setText(s->get("PostExitCommand").toString());
}

void JavaPage::on_javaDetectBtn_clicked()
{
	JavaVersionPtr java;

	VersionSelectDialog vselect(MMC->javalist().get(), tr("Select a Java version"), this, true);
	vselect.setResizeOn(2);
	vselect.exec();

	if (vselect.result() == QDialog::Accepted && vselect.selectedVersion())
	{
		java = std::dynamic_pointer_cast<JavaVersion>(vselect.selectedVersion());
		ui->javaPathTextBox->setText(java->path);
	}
}
void JavaPage::on_javaBrowseBtn_clicked()
{
	QString dir = QFileDialog::getOpenFileName(this, tr("Find Java executable"));
	if (!dir.isNull())
	{
		ui->javaPathTextBox->setText(dir);
	}
}
void JavaPage::on_javaTestBtn_clicked()
{
	checker.reset(new JavaChecker());
	connect(checker.get(), SIGNAL(checkFinished(JavaCheckResult)), this,
			SLOT(checkFinished(JavaCheckResult)));
	checker->path = ui->javaPathTextBox->text();
	checker->performCheck();
}

void JavaPage::checkFinished(JavaCheckResult result)
{
	if (result.valid)
	{
		QString text;
		text += "Java test succeeded!\n";
		if (result.is_64bit)
			text += "Using 64bit java.\n";
		text += "\n";
		text += "Platform reported: " + result.realPlatform + "\n";
		text += "Java version reported: " + result.javaVersion;
		QMessageBox::information(this, tr("Java test success"), text);
	}
	else
	{
		QMessageBox::warning(
			this, tr("Java test failure"),
			tr("The specified java binary didn't work. You should use the auto-detect feature, "
			   "or set the path to the java executable."));
	}
}
