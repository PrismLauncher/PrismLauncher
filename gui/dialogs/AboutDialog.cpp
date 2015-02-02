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

#include "AboutDialog.h"
#include "ui_AboutDialog.h"
#include <QIcon>
#include "MultiMC.h"
#include "gui/Platform.h"
#include "BuildConfig.h"

#include <logic/net/NetJob.h>

// Credits
// This is a hack, but I can't think of a better way to do this easily without screwing with QTextDocument...
QString getCreditsHtml(QStringList patrons)
{
	QString creditsHtml = QObject::tr(
		"<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.0//EN' 'http://www.w3.org/TR/REC-html40/strict.dtd'>"
		"<html>"
		""
		"<head>"
		"<meta name='qrichtext' content='1' />"
		"<style type='text/css'>"
		"p { white-space: pre-wrap; margin-top:2px; margin-bottom:2px; }"
		"</style>"
		"</head>"
		""
		"<body style=' font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;'>"
		""
		"<h3>MultiMC Developers</h3>"
		"<p>Andrew Okin &lt;<a href='mailto:forkk@forkk.net'>forkk@forkk.net</a>&gt;</p>"
		"<p>Petr Mrázek &lt;<a href='mailto:peterix@gmail.com'>peterix@gmail.com</a>&gt;</p>"
		"<p>Sky Welch &lt;<a href='mailto:multimc@bunnies.io'>multimc@bunnies.io</a>&gt;</p>"
		"<p>Jan (02JanDal) &lt;<a href='mailto:02jandal@gmail.com'>02jandal@gmail.com</a>&gt;</p>"
		""
		"<h3>With thanks to</h3>"
		"<p>Orochimarufan &lt;<a href='mailto:orochimarufan.x3@gmail.com'>orochimarufan.x3@gmail.com</a>&gt;</p>"
		"<p>TakSuyu &lt;<a href='mailto:taksuyu@gmail.com'>taksuyu@gmail.com</a>&gt;</p>"
		"<p>Kilobyte &lt;<a href='mailto:stiepen22@gmx.de'>stiepen22@gmx.de</a>&gt;</p>"
		"<p>Robotbrain &lt;<a href='https://twitter.com/skylordelros'>@skylordelros</a>&gt;</p>"
		"<p>Rootbear75 &lt;<a href='https://twitter.com/rootbear75'>@rootbear75</a>&gt; (build server)</p>"
		""
		"<h3>Patreon Patrons</h3>"
		"%1"
		""
		"</body>"
		"</html>");
	if (patrons.isEmpty())
		return creditsHtml.arg(QObject::tr("<p>Loading...</p>"));
	else
	{
		QString patronsStr;
		for (QString patron : patrons)
		{
			patronsStr.append(QString("<p>%1</p>").arg(patron));
		}

		return creditsHtml.arg(patronsStr);
	}
}

AboutDialog::AboutDialog(QWidget *parent) : QDialog(parent), ui(new Ui::AboutDialog)
{
	MultiMCPlatform::fixWM_CLASS(this);
	ui->setupUi(this);

	QString chtml = getCreditsHtml(QStringList());
	ui->creditsText->setHtml(chtml);

	ui->urlLabel->setOpenExternalLinks(true);

	ui->icon->setPixmap(QIcon::fromTheme("multimc").pixmap(64));
	ui->title->setText("MultiMC 5 " + BuildConfig.printableVersionString());

	ui->versionLabel->setText(tr("Version") +": " + BuildConfig.printableVersionString());
	ui->vtypeLabel->setText(tr("Version Type") +": " + BuildConfig.versionTypeName());
	ui->platformLabel->setText(tr("Platform") +": " + BuildConfig.BUILD_PLATFORM);

	if (BuildConfig.VERSION_BUILD >= 0)
		ui->buildNumLabel->setText(tr("Build Number") +": " + QString::number(BuildConfig.VERSION_BUILD));
	else
		ui->buildNumLabel->setVisible(false);

	if (!BuildConfig.VERSION_CHANNEL.isEmpty())
		ui->channelLabel->setText(tr("Channel") +": " + BuildConfig.VERSION_CHANNEL);
	else
		ui->channelLabel->setVisible(false);

	connect(ui->closeButton, SIGNAL(clicked()), SLOT(close()));

	MMC->connect(ui->aboutQt, SIGNAL(clicked()), SLOT(aboutQt()));

	loadPatronList();
}

AboutDialog::~AboutDialog()
{
	delete ui;
}

void AboutDialog::loadPatronList()
{
	NetJob* job = new NetJob("Patreon Patron List");
	patronListDownload = ByteArrayDownload::make(QUrl("http://files.multimc.org/patrons.txt"));
	job->addNetAction(patronListDownload);
	connect(job, &NetJob::succeeded, this, &AboutDialog::patronListLoaded);
	job->start();
}

void AboutDialog::patronListLoaded()
{
	QString patronListStr(patronListDownload->m_data);
	QString html = getCreditsHtml(patronListStr.split("\n", QString::SkipEmptyParts));
	ui->creditsText->setHtml(html);
}

