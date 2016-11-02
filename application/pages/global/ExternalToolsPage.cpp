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

#include "ExternalToolsPage.h"
#include "ui_ExternalToolsPage.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>

#include "settings/SettingsObject.h"
#include "tools/BaseProfiler.h"
#include <FileSystem.h>
#include "MultiMC.h"
#include <tools/MCEditTool.h>

ExternalToolsPage::ExternalToolsPage(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::ExternalToolsPage)
{
	ui->setupUi(this);
	ui->tabWidget->tabBar()->hide();

	#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
	ui->jsonEditorTextBox->setClearButtonEnabled(true);
	#endif

	ui->mceditLink->setOpenExternalLinks(true);
	ui->jvisualvmLink->setOpenExternalLinks(true);
	ui->jprofilerLink->setOpenExternalLinks(true);
	loadSettings();
}

ExternalToolsPage::~ExternalToolsPage()
{
	delete ui;
}

void ExternalToolsPage::loadSettings()
{
	auto s = MMC->settings();
	ui->jprofilerPathEdit->setText(s->get("JProfilerPath").toString());
	ui->jvisualvmPathEdit->setText(s->get("JVisualVMPath").toString());
	ui->mceditPathEdit->setText(s->get("MCEditPath").toString());

	// Editors
	ui->jsonEditorTextBox->setText(s->get("JsonEditor").toString());
}
void ExternalToolsPage::applySettings()
{
	auto s = MMC->settings();

	s->set("JProfilerPath", ui->jprofilerPathEdit->text());
	s->set("JVisualVMPath", ui->jvisualvmPathEdit->text());
	s->set("MCEditPath", ui->mceditPathEdit->text());

	// Editors
	QString jsonEditor = ui->jsonEditorTextBox->text();
	if (!jsonEditor.isEmpty() &&
		(!QFileInfo(jsonEditor).exists() || !QFileInfo(jsonEditor).isExecutable()))
	{
		QString found = QStandardPaths::findExecutable(jsonEditor);
		if (!found.isEmpty())
		{
			jsonEditor = found;
		}
	}
	s->set("JsonEditor", jsonEditor);
}

void ExternalToolsPage::on_jprofilerPathBtn_clicked()
{
	QString raw_dir = ui->jprofilerPathEdit->text();
	QString error;
	do
	{
		raw_dir = QFileDialog::getExistingDirectory(this, tr("JProfiler Directory"), raw_dir);
		if (raw_dir.isEmpty())
		{
			break;
		}
		QString cooked_dir = FS::NormalizePath(raw_dir);
		if (!MMC->profilers()["jprofiler"]->check(cooked_dir, &error))
		{
			QMessageBox::critical(this, tr("Error"), tr("Error while checking JProfiler install:\n%1").arg(error));
			continue;
		}
		else
		{
			ui->jprofilerPathEdit->setText(cooked_dir);
			break;
		}
	} while (1);
}
void ExternalToolsPage::on_jprofilerCheckBtn_clicked()
{
	QString error;
	if (!MMC->profilers()["jprofiler"]->check(ui->jprofilerPathEdit->text(), &error))
	{
		QMessageBox::critical(this, tr("Error"), tr("Error while checking JProfiler install:\n%1").arg(error));
	}
	else
	{
		QMessageBox::information(this, tr("OK"), tr("JProfiler setup seems to be OK"));
	}
}

void ExternalToolsPage::on_jvisualvmPathBtn_clicked()
{
	QString raw_dir = ui->jvisualvmPathEdit->text();
	QString error;
	do
	{
		raw_dir = QFileDialog::getOpenFileName(this, tr("JVisualVM Executable"), raw_dir);
		if (raw_dir.isEmpty())
		{
			break;
		}
		QString cooked_dir = FS::NormalizePath(raw_dir);
		if (!MMC->profilers()["jvisualvm"]->check(cooked_dir, &error))
		{
			QMessageBox::critical(this, tr("Error"), tr("Error while checking JVisualVM install:\n%1").arg(error));
			continue;
		}
		else
		{
			ui->jvisualvmPathEdit->setText(cooked_dir);
			break;
		}
	} while (1);
}
void ExternalToolsPage::on_jvisualvmCheckBtn_clicked()
{
	QString error;
	if (!MMC->profilers()["jvisualvm"]->check(ui->jvisualvmPathEdit->text(), &error))
	{
		QMessageBox::critical(this, tr("Error"), tr("Error while checking JVisualVM install:\n%1").arg(error));
	}
	else
	{
		QMessageBox::information(this, tr("OK"), tr("JVisualVM setup seems to be OK"));
	}
}

void ExternalToolsPage::on_mceditPathBtn_clicked()
{
	QString raw_dir = ui->mceditPathEdit->text();
	QString error;
	do
	{
#ifdef Q_OS_OSX
#warning stuff
		raw_dir = QFileDialog::getOpenFileName(this, tr("MCEdit Application"), raw_dir);
#else
		raw_dir = QFileDialog::getExistingDirectory(this, tr("MCEdit Directory"), raw_dir);
#endif
		if (raw_dir.isEmpty())
		{
			break;
		}
		QString cooked_dir = FS::NormalizePath(raw_dir);
		if (!MMC->mcedit()->check(cooked_dir, error))
		{
			QMessageBox::critical(this, tr("Error"), tr("Error while checking MCEdit install:\n%1").arg(error));
			continue;
		}
		else
		{
			ui->mceditPathEdit->setText(cooked_dir);
			break;
		}
	} while (1);
}
void ExternalToolsPage::on_mceditCheckBtn_clicked()
{
	QString error;
	if (!MMC->mcedit()->check(ui->mceditPathEdit->text(), error))
	{
		QMessageBox::critical(this, tr("Error"), tr("Error while checking MCEdit install:\n%1").arg(error));
	}
	else
	{
		QMessageBox::information(this, tr("OK"), tr("MCEdit setup seems to be OK"));
	}
}

void ExternalToolsPage::on_jsonEditorBrowseBtn_clicked()
{
	QString raw_file = QFileDialog::getOpenFileName(
		this, tr("JSON Editor"),
		ui->jsonEditorTextBox->text().isEmpty()
#if defined(Q_OS_LINUX)
				? QString("/usr/bin")
#else
			? QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation).first()
#endif
			: ui->jsonEditorTextBox->text());
	QString cooked_file = FS::NormalizePath(raw_file);

	if (cooked_file.isEmpty())
	{
		return;
	}

	// it has to exist and be an executable
	if (QFileInfo(cooked_file).exists() && QFileInfo(cooked_file).isExecutable())
	{
		ui->jsonEditorTextBox->setText(cooked_file);
	}
	else
	{
		QMessageBox::warning(this, tr("Invalid"),
							 tr("The file chosen does not seem to be an executable"));
	}
}

bool ExternalToolsPage::apply()
{
	applySettings();
	return true;
}
