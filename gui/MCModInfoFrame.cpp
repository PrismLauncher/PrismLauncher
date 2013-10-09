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

#include "MCModInfoFrame.h"
#include "ui_MCModInfoFrame.h"
void MCModInfoFrame::updateWithMod(Mod &m)
{
	if(m.type() == m.MOD_FOLDER)
	{
		clear();
		return;
	}

	QString missing = tr("Missing from mcmod.info");

	QString name = m.name();
	if(name.isEmpty()) name = missing;
	QString description = m.description();
	if(description.isEmpty()) description = missing;
	QString authors = m.authors();
	if(authors.isEmpty()) authors = missing;
	QString credits = m.credits();
	if(credits.isEmpty()) credits = missing;
	QString website = m.homeurl();
	if(website.isEmpty()) website = missing;
	else website = "<a href=\"" + website + "\">" + website + "</a>";

	setName(name);
	setDescription(description);
	setAuthors(authors);
	setCredits(credits);
	setWebsite(website);
}

void MCModInfoFrame::clear()
{
	setName(tr("Select a mod to view information..."));
	setDescription(tr("Mod description"));
	setAuthors(tr("Mod authors"));
	setCredits(tr("Mod credits"));
	setWebsite(tr("Mod website"));
}

MCModInfoFrame::MCModInfoFrame(QWidget *parent) :
	QFrame(parent),
	ui(new Ui::MCModInfoFrame)
{
	ui->setupUi(this);
}

MCModInfoFrame::~MCModInfoFrame()
{
	delete ui;
}

void MCModInfoFrame::setName(QString name)
{
	ui->label_Name->setText(name);
	//ui->label_Name->setToolTip(name);
}

void MCModInfoFrame::setDescription(QString description)
{
	ui->label_Description->setText(description);
	//ui->label_Description->setToolTip(description);
}

void MCModInfoFrame::setAuthors(QString authors)
{
	ui->label_Authors->setText(authors);
	//ui->label_Authors->setToolTip(authors);
}

void MCModInfoFrame::setCredits(QString credits)
{
	ui->label_Credits->setText(credits);
	//ui->label_Credits->setToolTip(credits);
}

void MCModInfoFrame::setWebsite(QString website)
{
	ui->label_Website->setText(website);
}
