/* Copyright 2013-2015 MultiMC Contributors
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
#include "JavaCommon.h"
#include "ui_JavaPage.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDir>

#include "dialogs/VersionSelectDialog.h"
#include <ColumnResizer.h>

#include "java/JavaUtils.h"
#include "java/JavaVersionList.h"

#include "settings/SettingsObject.h"
#include <FileSystem.h>
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
	JavaCommon::checkJVMArgs(s->get("JvmArgs").toString(), this->parentWidget());

	// Custom Commands
	s->set("PreLaunchCommand", ui->preLaunchCmdTextBox->text());
	s->set("WrapperCommand", ui->wrapperCmdTextBox->text());
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
	ui->wrapperCmdTextBox->setText(s->get("WrapperCommand").toString());
	ui->postExitCmdTextBox->setText(s->get("PostExitCommand").toString());
}

void JavaPage::on_javaDetectBtn_clicked()
{
	JavaVersionPtr java;

	VersionSelectDialog vselect(MMC->javalist().get(), tr("Select a Java version"), this, true);
	vselect.exec();

	if (vselect.result() == QDialog::Accepted && vselect.selectedVersion())
	{
		java = std::dynamic_pointer_cast<JavaVersion>(vselect.selectedVersion());
		ui->javaPathTextBox->setText(java->path);
	}
}

void JavaPage::on_javaBrowseBtn_clicked()
{
	QString raw_path = QFileDialog::getOpenFileName(this, tr("Find Java executable"));
	QString cooked_path = FS::NormalizePath(raw_path);

	// do not allow current dir - it's dirty. Do not allow dirs that don't exist
	if(cooked_path.isEmpty())
	{
		return;
	}

	QFileInfo javaInfo(cooked_path);;
	if(!javaInfo.exists() || !javaInfo.isExecutable())
	{
		return;
	}
	ui->javaPathTextBox->setText(cooked_path);
}

void JavaPage::on_javaTestBtn_clicked()
{
	if(checker)
	{
		return;
	}
	checker.reset(new JavaCommon::TestCheck(
		this, ui->javaPathTextBox->text(), ui->jvmArgsTextBox->text(),
		ui->minMemSpinBox->value(), ui->maxMemSpinBox->value(), ui->permGenSpinBox->value()));
	connect(checker.get(), SIGNAL(finished()), SLOT(checkerFinished()));
	checker->run();
}

void JavaPage::checkerFinished()
{
	checker.reset();
}
