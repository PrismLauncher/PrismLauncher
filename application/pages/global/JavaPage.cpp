/* Copyright 2013-2018 MultiMC Contributors
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
#include <QTabBar>

#include "dialogs/VersionSelectDialog.h"

#include "java/JavaUtils.h"
#include "java/JavaInstallList.h"

#include "settings/SettingsObject.h"
#include <FileSystem.h>
#include "MultiMC.h"
#include <sys.h>

JavaPage::JavaPage(QWidget *parent) : QWidget(parent), ui(new Ui::JavaPage)
{
	ui->setupUi(this);
	ui->tabWidget->tabBar()->hide();

	auto sysMB = Sys::getSystemRam() / Sys::megabyte;
	ui->maxMemSpinBox->setMaximum(sysMB);
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
	int min = ui->minMemSpinBox->value();
	int max = ui->maxMemSpinBox->value();
	if(min < max)
	{
		s->set("MinMemAlloc", min);
		s->set("MaxMemAlloc", max);
	}
	else
	{
		s->set("MinMemAlloc", max);
		s->set("MaxMemAlloc", min);
	}
	s->set("PermGen", ui->permGenSpinBox->value());

	// Java Settings
	s->set("JavaPath", ui->javaPathTextBox->text());
	s->set("JvmArgs", ui->jvmArgsTextBox->text());
	JavaCommon::checkJVMArgs(s->get("JvmArgs").toString(), this->parentWidget());
}
void JavaPage::loadSettings()
{
	auto s = MMC->settings();
	// Memory
	int min = s->get("MinMemAlloc").toInt();
	int max = s->get("MaxMemAlloc").toInt();
	if(min < max)
	{
		ui->minMemSpinBox->setValue(min);
		ui->maxMemSpinBox->setValue(max);
	}
	else
	{
		ui->minMemSpinBox->setValue(max);
		ui->maxMemSpinBox->setValue(min);
	}
	ui->permGenSpinBox->setValue(s->get("PermGen").toInt());

	// Java Settings
	ui->javaPathTextBox->setText(s->get("JavaPath").toString());
	ui->jvmArgsTextBox->setText(s->get("JvmArgs").toString());
}

void JavaPage::on_javaDetectBtn_clicked()
{
	JavaInstallPtr java;

	VersionSelectDialog vselect(MMC->javalist().get(), tr("Select a Java version"), this, true);
	vselect.setResizeOn(2);
	vselect.exec();

	if (vselect.result() == QDialog::Accepted && vselect.selectedVersion())
	{
		java = std::dynamic_pointer_cast<JavaInstall>(vselect.selectedVersion());
		ui->javaPathTextBox->setText(java->path);
	}
}

void JavaPage::on_javaBrowseBtn_clicked()
{
	QString raw_path = QFileDialog::getOpenFileName(this, tr("Find Java executable"));

	// do not allow current dir - it's dirty. Do not allow dirs that don't exist
	if(raw_path.isEmpty())
	{
		return;
	}

	QString cooked_path = FS::NormalizePath(raw_path);
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
