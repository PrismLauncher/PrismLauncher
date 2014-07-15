/* Copyright 2014 MultiMC Contributors
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

#include <pathutils.h>

#include "logic/settings/SettingsObject.h"
#include "logic/tools/BaseProfiler.h"
#include "MultiMC.h"

ExternalToolsPage::ExternalToolsPage(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::ExternalToolsPage)
{
	ui->setupUi(this);

	ui->mceditLink->setOpenExternalLinks(true);
	ui->jvisualvmLink->setOpenExternalLinks(true);
	ui->jprofilerLink->setOpenExternalLinks(true);
}

ExternalToolsPage::~ExternalToolsPage()
{
	delete ui;
}

void ExternalToolsPage::loadSettings(SettingsObject *object)
{
	ui->jprofilerPathEdit->setText(object->get("JProfilerPath").toString());
	ui->jvisualvmPathEdit->setText(object->get("JVisualVMPath").toString());
	ui->mceditPathEdit->setText(object->get("MCEditPath").toString());
}
void ExternalToolsPage::applySettings(SettingsObject *object)
{
	object->set("JProfilerPath", ui->jprofilerPathEdit->text());
	object->set("JVisualVMPath", ui->jvisualvmPathEdit->text());
	object->set("MCEditPath", ui->mceditPathEdit->text());
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
		QString cooked_dir = NormalizePath(raw_dir);
		if (!MMC->profilers()["jprofiler"]->check(cooked_dir, &error))
		{
			QMessageBox::critical(this, tr("Error"),
								  tr("Error while checking JProfiler install:\n%1").arg(error));
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
		QMessageBox::critical(this, tr("Error"),
							  tr("Error while checking JProfiler install:\n%1").arg(error));
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
		QString cooked_dir = NormalizePath(raw_dir);
		if (!MMC->profilers()["jvisualvm"]->check(cooked_dir, &error))
		{
			QMessageBox::critical(this, tr("Error"),
								  tr("Error while checking JVisualVM install:\n%1").arg(error));
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
		QMessageBox::critical(this, tr("Error"),
							  tr("Error while checking JVisualVM install:\n%1").arg(error));
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
		QString cooked_dir = NormalizePath(raw_dir);
		if (!MMC->tools()["mcedit"]->check(cooked_dir, &error))
		{
			QMessageBox::critical(this, tr("Error"),
								  tr("Error while checking MCEdit install:\n%1").arg(error));
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
	if (!MMC->tools()["mcedit"]->check(ui->mceditPathEdit->text(), &error))
	{
		QMessageBox::critical(this, tr("Error"),
							  tr("Error while checking MCEdit install:\n%1").arg(error));
	}
	else
	{
		QMessageBox::information(this, tr("OK"), tr("MCEdit setup seems to be OK"));
	}
}
