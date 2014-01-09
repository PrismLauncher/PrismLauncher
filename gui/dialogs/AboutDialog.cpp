/* Copyright 2013 MultiMC Contributors
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

#include "AboutDialog.h"
#include "ui_AboutDialog.h"
#include <QIcon>
#include "MultiMC.h"
#include "gui/Platform.h"

AboutDialog::AboutDialog(QWidget *parent) : QDialog(parent), ui(new Ui::AboutDialog)
{
	MultiMCPlatform::fixWM_CLASS(this);
	ui->setupUi(this);

	ui->icon->setPixmap(QIcon(":/icons/multimc/scalable/apps/multimc.svg").pixmap(64));
	ui->title->setText("MultiMC 5 " + MMC->version().toString());

	ui->versionLabel->setText(tr("Version") +": " + MMC->version().toString());
	ui->vtypeLabel->setText(tr("Version Type") +": " + MMC->version().typeName());
	ui->platformLabel->setText(tr("Platform") +": " + MMC->version().platform);

	if (MMC->version().build >= 0)
		ui->buildNumLabel->setText(tr("Build Number") +": " + QString::number(MMC->version().build));
	else
		ui->buildNumLabel->setVisible(false);

	if (!MMC->version().channel.isEmpty())
		ui->channelLabel->setText(tr("Channel") +": " + MMC->version().channel);
	else
		ui->channelLabel->setVisible(false);

	connect(ui->closeButton, SIGNAL(clicked()), SLOT(close()));

	MMC->connect(ui->aboutQt, SIGNAL(clicked()), SLOT(aboutQt()));
}

AboutDialog::~AboutDialog()
{
	delete ui;
}
