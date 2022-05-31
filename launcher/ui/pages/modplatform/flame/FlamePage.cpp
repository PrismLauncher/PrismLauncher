// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
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

#include "FlamePage.h"
#include "ui_FlamePage.h"

#include <QKeyEvent>

#include "Application.h"
#include "FlameModel.h"
#include "InstanceImportTask.h"
#include "Json.h"
#include "ui/dialogs/NewInstanceDialog.h"

FlamePage::FlamePage(NewInstanceDialog* dialog, QWidget* parent) : QWidget(parent), ui(new Ui::FlamePage), dialog(dialog)
{
    ui->setupUi(this);
    connect(ui->searchButton, &QPushButton::clicked, this, &FlamePage::triggerSearch);
    ui->searchEdit->installEventFilter(this);
    listModel = new Flame::ListModel(this);
    ui->packView->setModel(listModel);

    ui->versionSelectionBox->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->versionSelectionBox->view()->parentWidget()->setMaximumHeight(300);

    // index is used to set the sorting with the curseforge api
    ui->sortByBox->addItem(tr("Sort by Featured"));
    ui->sortByBox->addItem(tr("Sort by Popularity"));
    ui->sortByBox->addItem(tr("Sort by Last Updated"));
    ui->sortByBox->addItem(tr("Sort by Name"));
    ui->sortByBox->addItem(tr("Sort by Author"));
    ui->sortByBox->addItem(tr("Sort by Total Downloads"));

    connect(ui->sortByBox, SIGNAL(currentIndexChanged(int)), this, SLOT(triggerSearch()));
    connect(ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &FlamePage::onSelectionChanged);
    connect(ui->versionSelectionBox, &QComboBox::currentTextChanged, this, &FlamePage::onVersionSelectionChanged);
}

FlamePage::~FlamePage()
{
    delete ui;
}

bool FlamePage::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == ui->searchEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return) {
            triggerSearch();
            keyEvent->accept();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

bool FlamePage::shouldDisplay() const
{
    return true;
}

void FlamePage::retranslate()
{
    ui->retranslateUi(this);
}

void FlamePage::openedImpl()
{
    suggestCurrent();
    triggerSearch();
}

void FlamePage::triggerSearch()
{
    listModel->searchWithTerm(ui->searchEdit->text(), ui->sortByBox->currentIndex());
}

void FlamePage::onSelectionChanged(QModelIndex first, QModelIndex second)
{
    ui->versionSelectionBox->clear();

    if (!first.isValid()) {
        if (isOpened) {
            dialog->setSuggestedPack();
        }
        return;
    }

    current = listModel->data(first, Qt::UserRole).value<Flame::IndexedPack>();
    QString text = "";
    QString name = current.name;

    if (current.websiteUrl.isEmpty())
        text = name;
    else
        text = "<a href=\"" + current.websiteUrl + "\">" + name + "</a>";
    if (!current.authors.empty()) {
        auto authorToStr = [](Flame::ModpackAuthor& author) {
            if (author.url.isEmpty()) {
                return author.name;
            }
            return QString("<a href=\"%1\">%2</a>").arg(author.url, author.name);
        };
        QStringList authorStrs;
        for (auto& author : current.authors) {
            authorStrs.push_back(authorToStr(author));
        }
        text += "<br>" + tr(" by ") + authorStrs.join(", ");
    }
    text += "<br><br>";

    ui->packDescription->setHtml(text + current.description);

    if (current.versionsLoaded == false) {
        qDebug() << "Loading flame modpack versions";
        auto netJob = new NetJob(QString("Flame::PackVersions(%1)").arg(current.name), APPLICATION->network());
        auto response = new QByteArray();
        int addonId = current.addonId;
        netJob->addNetAction(Net::Download::makeByteArray(QString("https://api.curseforge.com/v1/mods/%1/files").arg(addonId), response));

        QObject::connect(netJob, &NetJob::succeeded, this, [this, response, addonId] {
            if (addonId != current.addonId) {
                return;  // wrong request
            }
            QJsonParseError parse_error;
            QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
            if (parse_error.error != QJsonParseError::NoError) {
                qWarning() << "Error while parsing JSON response from CurseForge at " << parse_error.offset
                           << " reason: " << parse_error.errorString();
                qWarning() << *response;
                return;
            }
            auto arr = Json::ensureArray(doc.object(), "data");
            try {
                Flame::loadIndexedPackVersions(current, arr);
            } catch (const JSONValidationError& e) {
                qDebug() << *response;
                qWarning() << "Error while reading flame modpack version: " << e.cause();
            }

            for (auto version : current.versions) {
                ui->versionSelectionBox->addItem(version.version, QVariant(version.downloadUrl));
            }

            suggestCurrent();
        });
        QObject::connect(netJob, &NetJob::finished, this, [response, netJob] {
            netJob->deleteLater();
            delete response;
        });
        netJob->start();
    } else {
        for (auto version : current.versions) {
            ui->versionSelectionBox->addItem(version.version, QVariant(version.downloadUrl));
        }

        suggestCurrent();
    }
}

void FlamePage::suggestCurrent()
{
    if (!isOpened) {
        return;
    }

    if (selectedVersion.isEmpty()) {
        dialog->setSuggestedPack();
        return;
    }

    dialog->setSuggestedPack(current.name, new InstanceImportTask(selectedVersion,this));
    QString editedLogoName;
    editedLogoName = "curseforge_" + current.logoName.section(".", 0, 0);
    listModel->getLogo(current.logoName, current.logoUrl,
                       [this, editedLogoName](QString logo) { dialog->setSuggestedIconFromFile(logo, editedLogoName); });
}

void FlamePage::onVersionSelectionChanged(QString data)
{
    if (data.isNull() || data.isEmpty()) {
        selectedVersion = "";
        return;
    }
    selectedVersion = ui->versionSelectionBox->currentData().toString();
    suggestCurrent();
}
