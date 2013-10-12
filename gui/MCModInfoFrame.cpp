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
#include <QMessageBox>
#include <QtGui>
void MCModInfoFrame::updateWithMod(Mod &m)
{
	if(m.type() == m.MOD_FOLDER)
	{
		clear();
		return;
	}

	QString text = "";
	QString name = "";
	if(m.name().isEmpty()) name = m.id();
	else name = m.name();

	if(m.homeurl().isEmpty()) text = name;
	else text = "<a href=\"" + m.homeurl() + "\">" + name + "</a>";
	if(!m.authors().isEmpty()) text += " by " + m.authors();

	setModText(text);

	if(m.description().isEmpty())
	{
		setModDescription(tr("No description provided in mcmod.info"));
	}
	else
	{
		setModDescription(m.description());
	}
}

void MCModInfoFrame::clear()
{
	setModText(tr("Select a mod to view title and authors..."));
	setModDescription(tr("Select a mod to view description..."));
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

void MCModInfoFrame::setModText(QString text)
{
	ui->label_ModText->setText(text);
}

void MCModInfoFrame::setModDescription(QString text)
{
    ui->label_ModDescription->setToolTip("");
    QString intermediatetext = text.trimmed();
    bool prev(false);
    QChar rem('\n');
    QString finaltext;
    finaltext.reserve(intermediatetext.size());
    foreach(const QChar& c, intermediatetext)
    {
        if(c == rem && prev){
            continue;
        }
        prev = c == rem;
        finaltext += c;
    }
    QString labeltext;
    labeltext.reserve(300);
    if(finaltext.length() > 290)
    {
        ui->label_ModDescription->setOpenExternalLinks(false);
        labeltext.append(finaltext.left(287) + "<a href=\"\">...</a>");
        QObject::connect(ui->label_ModDescription, &QLabel::linkActivated, this, &MCModInfoFrame::modDescEllipsisHandler);
    }
    else
    {
        labeltext.append(finaltext);
    }
    ui->label_ModDescription->setText(labeltext);
}
void MCModInfoFrame::modDescEllipsisHandler(const QString &link)
{
    QMessageBox msgbox;
    msgbox.setDetailedText(desc);
    msgbox.exec();
}
