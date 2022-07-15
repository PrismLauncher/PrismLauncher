// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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

#include "ModPage.h"
#include "Application.h"
#include "ui_ModPage.h"

#include <QKeyEvent>
#include <memory>

#include <HoeDown.h>

#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "ui/dialogs/ModDownloadDialog.h"
#include "ui/widgets/ProjectItem.h"

ModPage::ModPage(ModDownloadDialog* dialog, BaseInstance* instance, ModAPI* api)
    : QWidget(dialog)
    , m_instance(instance)
    , ui(new Ui::ModPage)
    , dialog(dialog)
    , filter_widget(static_cast<MinecraftInstance*>(instance)->getPackProfile()->getComponentVersion("net.minecraft"), this)
    , api(api)
{
    ui->setupUi(this);
    connect(ui->searchButton, &QPushButton::clicked, this, &ModPage::triggerSearch);
    connect(ui->modFilterButton, &QPushButton::clicked, this, &ModPage::filterMods);
    ui->searchEdit->installEventFilter(this);

    ui->versionSelectionBox->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->versionSelectionBox->view()->parentWidget()->setMaximumHeight(300);

    ui->gridLayout_3->addWidget(&filter_widget, 0, 0, 1, ui->gridLayout_3->columnCount());

    filter_widget.setInstance(static_cast<MinecraftInstance*>(m_instance));
    m_filter = filter_widget.getFilter();

    connect(&filter_widget, &ModFilterWidget::filterChanged, this, [&]{
        ui->searchButton->setStyleSheet("text-decoration: underline");
    });
    connect(&filter_widget, &ModFilterWidget::filterUnchanged, this, [&]{
        ui->searchButton->setStyleSheet("text-decoration: none");
    });

    ui->packView->setItemDelegate(new ProjectItemDelegate(this));
    ui->packView->installEventFilter(this);
}

ModPage::~ModPage()
{
    delete ui;
}


/******** Qt things ********/

void ModPage::openedImpl()
{
    updateSelectionButton();
    triggerSearch();
}

auto ModPage::eventFilter(QObject* watched, QEvent* event) -> bool
{
    if (watched == ui->searchEdit && event->type() == QEvent::KeyPress) {
        auto* keyEvent = dynamic_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return) {
            triggerSearch();
            keyEvent->accept();
            return true;
        }
    } else if (watched == ui->packView && event->type() == QEvent::KeyPress) {
        auto* keyEvent = dynamic_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return) {
            onModSelected();

            // To have the 'select mod' button outlined instead of the 'review and confirm' one
            ui->modSelectionButton->setFocus(Qt::FocusReason::ShortcutFocusReason);
            ui->packView->setFocus(Qt::FocusReason::NoFocusReason);

            keyEvent->accept();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}


/******** Callbacks to events in the UI (set up in the derived classes) ********/

void ModPage::filterMods()
{
    filter_widget.setHidden(!filter_widget.isHidden());
}

void ModPage::triggerSearch()
{
    auto changed = filter_widget.changed();
    m_filter = filter_widget.getFilter();
    
    if(changed){
        ui->packView->clearSelection();
        ui->packDescription->clear();
        ui->versionSelectionBox->clear();
        updateSelectionButton();
    }

    listModel->searchWithTerm(getSearchTerm(), ui->sortByBox->currentIndex(), changed);
}

QString ModPage::getSearchTerm() const
{
    return ui->searchEdit->text();
}
void ModPage::setSearchTerm(QString term)
{
    ui->searchEdit->setText(term);
}

void ModPage::onSelectionChanged(QModelIndex first, QModelIndex second)
{
    ui->versionSelectionBox->clear();

    if (!first.isValid()) { return; }

    current = listModel->data(first, Qt::UserRole).value<ModPlatform::IndexedPack>();

    if (!current.versionsLoaded) {
        qDebug() << QString("Loading %1 mod versions").arg(debugName());

        ui->modSelectionButton->setText(tr("Loading versions..."));
        ui->modSelectionButton->setEnabled(false);

        listModel->requestModVersions(current);
    } else {
        for (int i = 0; i < current.versions.size(); i++) {
            ui->versionSelectionBox->addItem(current.versions[i].version, QVariant(i));
        }
        if (ui->versionSelectionBox->count() == 0) { ui->versionSelectionBox->addItem(tr("No valid version found."), QVariant(-1)); }

        updateSelectionButton();
    }

    if(!current.extraDataLoaded){
        qDebug() << QString("Loading %1 mod info").arg(debugName());
        listModel->requestModInfo(current);
    }

    updateUi();
}

void ModPage::onVersionSelectionChanged(QString data)
{
    if (data.isNull() || data.isEmpty()) {
        selectedVersion = -1;
        return;
    }
    selectedVersion = ui->versionSelectionBox->currentData().toInt();
    updateSelectionButton();
}

void ModPage::onModSelected()
{
    if (selectedVersion < 0)
        return;

    auto& version = current.versions[selectedVersion];
    if (dialog->isModSelected(current.name, version.fileName)) {
        dialog->removeSelectedMod(current.name);
    } else {
        bool is_indexed = !APPLICATION->settings()->get("ModMetadataDisabled").toBool();
        dialog->addSelectedMod(current.name, new ModDownloadTask(current, version, dialog->mods, is_indexed));
    }

    updateSelectionButton();
}


/******** Make changes to the UI ********/

void ModPage::retranslate()
{
   ui->retranslateUi(this);
}

void ModPage::updateModVersions(int prev_count)
{
    auto packProfile = (dynamic_cast<MinecraftInstance*>(m_instance))->getPackProfile();

    QString mcVersion = packProfile->getComponentVersion("net.minecraft");

    for (int i = 0; i < current.versions.size(); i++) {
        auto version = current.versions[i];
        bool valid = false;
        for(auto& mcVer : m_filter->versions){
            //NOTE: Flame doesn't care about loader, so passing it changes nothing.
            if (validateVersion(version, mcVer.toString(), packProfile->getModLoaders())) {
                valid = true;
                break;
            }
        }
        if(valid || m_filter->versions.size() == 0)
            ui->versionSelectionBox->addItem(version.version, QVariant(i));
    }
    if (ui->versionSelectionBox->count() == 0 && prev_count != 0) { 
        ui->versionSelectionBox->addItem(tr("No valid version found!"), QVariant(-1)); 
        ui->modSelectionButton->setText(tr("Cannot select invalid version :("));
    }

    updateSelectionButton();
}


void ModPage::updateSelectionButton()
{
    if (!isOpened || selectedVersion < 0) {
        ui->modSelectionButton->setEnabled(false);
        return;
    }

    ui->modSelectionButton->setEnabled(true);
    auto& version = current.versions[selectedVersion];
    if (!dialog->isModSelected(current.name, version.fileName)) {
        ui->modSelectionButton->setText(tr("Select mod for download"));
    } else {
        ui->modSelectionButton->setText(tr("Deselect mod for download"));
    }
}

void ModPage::updateUi()
{
    QString text = "";
    QString name = current.name;

    if (current.websiteUrl.isEmpty())
        text = name;
    else
        text = "<a href=\"" + current.websiteUrl + "\">" + name + "</a>";

    if (!current.authors.empty()) {
        auto authorToStr = [](ModPlatform::ModpackAuthor& author) -> QString {
            if (author.url.isEmpty()) { return author.name; }
            return QString("<a href=\"%1\">%2</a>").arg(author.url, author.name);
        };
        QStringList authorStrs;
        for (auto& author : current.authors) {
            authorStrs.push_back(authorToStr(author));
        }
        text += "<br>" + tr(" by ") + authorStrs.join(", ");
    }

    
    if(current.extraDataLoaded) {
        if (!current.extraData.donate.isEmpty()) {
            text += "<br><br>" + tr("Donate information: ");
            auto donateToStr = [](ModPlatform::DonationData& donate) -> QString {
                return QString("<a href=\"%1\">%2</a>").arg(donate.url, donate.platform);
            };
            QStringList donates;
            for (auto& donate : current.extraData.donate) {
                donates.append(donateToStr(donate));
            }
            text += donates.join(", ");
        }

        if (!current.extraData.issuesUrl.isEmpty()
         || !current.extraData.sourceUrl.isEmpty()
         || !current.extraData.wikiUrl.isEmpty()
         || !current.extraData.discordUrl.isEmpty()) {
            text += "<br><br>" + tr("External links:") + "<br>";
        }

        if (!current.extraData.issuesUrl.isEmpty())
            text += "- " + tr("Issues: <a href=%1>%1</a>").arg(current.extraData.issuesUrl) + "<br>";
        if (!current.extraData.wikiUrl.isEmpty())
            text += "- " + tr("Wiki: <a href=%1>%1</a>").arg(current.extraData.wikiUrl) + "<br>";
        if (!current.extraData.sourceUrl.isEmpty())
            text += "- " + tr("Source code: <a href=%1>%1</a>").arg(current.extraData.sourceUrl) + "<br>";
        if (!current.extraData.discordUrl.isEmpty())
            text += "- " + tr("Discord: <a href=%1>%1</a>").arg(current.extraData.discordUrl) + "<br>";
    }

    text += "<hr>";

    HoeDown h;
    ui->packDescription->setHtml(text + (current.extraData.body.isEmpty() ? current.description : h.process(current.extraData.body.toUtf8())));
}
