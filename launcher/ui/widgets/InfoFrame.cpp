/* Copyright 2013-2021 MultiMC Contributors
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

#include <QMessageBox>

#include "InfoFrame.h"
#include "ui_InfoFrame.h"

#include "ui/dialogs/CustomMessageBox.h"

InfoFrame::InfoFrame(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::InfoFrame)
{
    ui->setupUi(this);
    ui->descriptionLabel->setHidden(true);
    ui->nameLabel->setHidden(true);
    updateHiddenState();
}

InfoFrame::~InfoFrame()
{
    delete ui;
}

void InfoFrame::updateWithMod(Mod const& m)
{
    if (m.type() == ResourceType::FOLDER)
    {
        clear();
        return;
    }

    QString text = "";
    QString name = "";
    if (m.name().isEmpty())
        name = m.internal_id();
    else
        name = m.name();

    if (m.homeurl().isEmpty())
        text = name;
    else
        text = "<a href=\"" + m.homeurl() + "\">" + name + "</a>";
    if (!m.authors().isEmpty())
        text += " by " + m.authors().join(", ");

    setName(text);

    if (m.description().isEmpty())
    {
        setDescription(QString());
    }
    else
    {
        setDescription(m.description());
    }
}

void InfoFrame::updateWithResource(const Resource& resource)
{
    setName(resource.name());
}

void InfoFrame::clear()
{
    setName();
    setDescription();
}

void InfoFrame::updateHiddenState()
{
    if(ui->descriptionLabel->isHidden() && ui->nameLabel->isHidden())
    {
        setHidden(true);
    }
    else
    {
        setHidden(false);
    }
}

void InfoFrame::setName(QString text)
{
    if(text.isEmpty())
    {
        ui->nameLabel->setHidden(true);
    }
    else
    {
        ui->nameLabel->setText(text);
        ui->nameLabel->setHidden(false);
    }
    updateHiddenState();
}

void InfoFrame::setDescription(QString text)
{
    if(text.isEmpty())
    {
        ui->descriptionLabel->setHidden(true);
        updateHiddenState();
        return;
    }
    else
    {
        ui->descriptionLabel->setHidden(false);
        updateHiddenState();
    }
    ui->descriptionLabel->setToolTip("");
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
        ui->descriptionLabel->setOpenExternalLinks(false);
        ui->descriptionLabel->setTextFormat(Qt::TextFormat::RichText);
        m_description = text;
        // This allows injecting HTML here.
        labeltext.append("<html><body>" + finaltext.left(287) + "<a href=\"#mod_desc\">...</a></body></html>");
        QObject::connect(ui->descriptionLabel, &QLabel::linkActivated, this, &InfoFrame::descriptionEllipsisHandler);
    }
    else
    {
        ui->descriptionLabel->setTextFormat(Qt::TextFormat::PlainText);
        labeltext.append(finaltext);
    }
    ui->descriptionLabel->setText(labeltext);
}

void InfoFrame::descriptionEllipsisHandler(QString link)
{
    if(!m_current_box)
    {
        m_current_box = CustomMessageBox::selectable(this, "", m_description);
        connect(m_current_box, &QMessageBox::finished, this, &InfoFrame::boxClosed);
        m_current_box->show();
    }
    else
    {
        m_current_box->setText(m_description);
    }
}

void InfoFrame::boxClosed(int result)
{
    m_current_box = nullptr;
}
