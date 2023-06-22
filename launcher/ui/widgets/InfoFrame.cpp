// SPDX-License-Identifier: GPL-3.0-only
/*
*  PolyMC - Minecraft Launcher
*  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, version 3.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*
* This file incorporates work covered by the following copyright and
* permission notice:
*
*      Copyright 2013-2021 MultiMC Contributors
*
*      Licensed under the Apache License, Version 2.0 (the "License");
*      you may not use this file except in compliance with the License.
*      You may obtain a copy of the License at
*
*          http://www.apache.org/licenses/LICENSE-2.0
*
*      Unless required by applicable law or agreed to in writing, software
*      distributed under the License is distributed on an "AS IS" BASIS,
*      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*      See the License for the specific language governing permissions and
*      limitations under the License.
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
    ui->licenseLabel->setHidden(true);
    ui->issueTrackerLabel->setHidden(true);
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

    setImage(m.icon({64,64}));

    auto licenses = m.licenses();
    QString licenseText = "";
    if (!licenses.empty()) {
        for (auto l : licenses) {
            if (!licenseText.isEmpty()) {
                licenseText += "\n"; // add newline between licenses
            }
            if (!l.name.isEmpty()) {
                if (l.url.isEmpty()) {
                    licenseText += l.name;
                } else {
                    licenseText += "<a href=\"" + l.url + "\">" + l.name + "</a>";
                }
            } else if (!l.url.isEmpty()) {
                licenseText += "<a href=\"" + l.url + "\">" + l.url + "</a>";
            }
            if (!l.description.isEmpty() && l.description != l.name) {
               licenseText += " " + l.description;
            }
        } 
    }
    if (!licenseText.isEmpty()) {
        setLicense(tr("License: %1").arg(licenseText));
    } else {
        setLicense();
    }

    QString issueTracker = "";
    if (!m.issueTracker().isEmpty()) {
        issueTracker += tr("Report issues to: ");
        issueTracker += "<a href=\"" + m.issueTracker() + "\">" + m.issueTracker() + "</a>";
    } 
    setIssueTracker(issueTracker);
}

void InfoFrame::updateWithResource(const Resource& resource)
{
    setName(resource.name());
    setImage();
}

QString InfoFrame::renderColorCodes(QString input) {
    // We have to manually set the colors for use.
    //
    // A color is set using §x, with x = a hex number from 0 to f.
    //
    // We traverse the description and, when one of those is found, we create
    // a span element with that color set.
    //
    // TODO: Wrap links inside <a> tags

    // https://minecraft.fandom.com/wiki/Formatting_codes#Color_codes
    const QMap<QChar, QString> color_codes_map = {
        {'0', "#000000"}, {'1', "#0000AA"}, {'2', "#00AA00"}, {'3', "#00AAAA"}, {'4', "#AA0000"},
        {'5', "#AA00AA"}, {'6', "#FFAA00"}, {'7', "#AAAAAA"}, {'8', "#555555"}, {'9', "#5555FF"},
        {'a', "#55FF55"}, {'b', "#55FFFF"}, {'c', "#FF5555"}, {'d', "#FF55FF"}, {'e', "#FFFF55"},
        {'f', "#FFFFFF"}
    };
    // https://minecraft.fandom.com/wiki/Formatting_codes#Formatting_codes
    const QMap<QChar, QString> formatting_codes_map = {
        {'l', "b"}, {'m', "s"}, {'n', "u"}, {'o', "i"}
    };

    QString html("<html>");
    QList<QString> tags{};

    auto it = input.constBegin();
    while (it != input.constEnd()) {
        // is current char § and is there a following char
        if (*it == u'§' && (it + 1) != input.constEnd()) {
            auto const& code = *(++it);  // incrementing here!

            auto const color_entry = color_codes_map.constFind(code);
            auto const tag_entry = formatting_codes_map.constFind(code);

            if (color_entry != color_codes_map.constEnd()) {  // color code
                html += QString("<span style=\"color: %1;\">").arg(color_entry.value());
                tags << "span";
            } else if (tag_entry != formatting_codes_map.constEnd()) {  // formatting code
                html += QString("<%1>").arg(tag_entry.value());
                tags << tag_entry.value();
            } else if (code == 'r') {  // reset all formatting
                while (!tags.isEmpty()) {
                    html += QString("</%1>").arg(tags.takeLast());
                }
            } else {  // pass unknown codes through
                html += QString("§%1").arg(code);
            }
        } else {
            html += *it;
        }
        it++;
    }
    while (!tags.isEmpty()) {
        html += QString("</%1>").arg(tags.takeLast());
    }
    html += "</html>";

    html.replace("\n", "<br>");
    return html;
}

void InfoFrame::updateWithResourcePack(ResourcePack& resource_pack)
{
    setName(renderColorCodes(resource_pack.name()));
    setDescription(renderColorCodes(resource_pack.description()));
    setImage(resource_pack.image({64, 64}));
}

void InfoFrame::updateWithTexturePack(TexturePack& texture_pack)
{
    setName(renderColorCodes(texture_pack.name()));
    setDescription(renderColorCodes(texture_pack.description()));
    setImage(texture_pack.image({64, 64}));
}

void InfoFrame::clear()
{
    setName();
    setDescription();
    setImage();
    setLicense();
    setIssueTracker();
}

void InfoFrame::updateHiddenState()
{
    if (ui->descriptionLabel->isHidden() && ui->nameLabel->isHidden() && ui->licenseLabel->isHidden() &&
        ui->issueTrackerLabel->isHidden()) {
        setHidden(true);
    } else {
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
        ui->descriptionLabel->setTextFormat(Qt::TextFormat::AutoText);
        labeltext.append(finaltext);
    }
    ui->descriptionLabel->setText(labeltext);
}

void InfoFrame::setLicense(QString text)
{
    if(text.isEmpty())
    {
        ui->licenseLabel->setHidden(true);
        updateHiddenState();
        return;
    }
    else
    {
        ui->licenseLabel->setHidden(false);
        updateHiddenState();
    }
    ui->licenseLabel->setToolTip("");
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
        ui->licenseLabel->setOpenExternalLinks(false);
        ui->licenseLabel->setTextFormat(Qt::TextFormat::RichText);
        m_description = text;
        // This allows injecting HTML here.
        labeltext.append("<html><body>" + finaltext.left(287) + "<a href=\"#mod_desc\">...</a></body></html>");
        QObject::connect(ui->licenseLabel, &QLabel::linkActivated, this, &InfoFrame::licenseEllipsisHandler);
    }
    else
    {
        ui->licenseLabel->setTextFormat(Qt::TextFormat::AutoText);
        labeltext.append(finaltext);
    }
    ui->licenseLabel->setText(labeltext);
}

void InfoFrame::setIssueTracker(QString text)
{
    if(text.isEmpty())
    {
        ui->issueTrackerLabel->setHidden(true);
    }
    else
    {
        ui->issueTrackerLabel->setText(text);
        ui->issueTrackerLabel->setHidden(false);
    }
    updateHiddenState();
}

void InfoFrame::setImage(QPixmap img)
{
    if (img.isNull()) {
        ui->iconLabel->setHidden(true);
    } else {
        ui->iconLabel->setHidden(false);
        ui->iconLabel->setPixmap(img);
    }
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

void InfoFrame::licenseEllipsisHandler(QString link)
{
    if(!m_current_box)
    {
        m_current_box = CustomMessageBox::selectable(this, "", m_license);
        connect(m_current_box, &QMessageBox::finished, this, &InfoFrame::boxClosed);
        m_current_box->show();
    }
    else
    {
        m_current_box->setText(m_license);
    }
}

void InfoFrame::boxClosed(int result)
{
    m_current_box = nullptr;
}
