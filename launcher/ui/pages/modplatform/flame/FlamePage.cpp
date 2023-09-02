// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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
#include "modplatform/flame/FlameAPI.h"
#include "ui/dialogs/NewInstanceDialog.h"
#include "ui/widgets/ProjectItem.h"

#include "net/ApiDownload.h"

static FlameAPI api;

FlamePage::FlamePage(NewInstanceDialog* dialog, QWidget* parent)
    : QWidget(parent), ui(new Ui::FlamePage), dialog(dialog), m_fetch_progress(this, false)
{
    ui->setupUi(this);
    connect(ui->searchButton, &QPushButton::clicked, this, &FlamePage::triggerSearch);
    ui->searchEdit->installEventFilter(this);
    listModel = new Flame::ListModel(this);
    ui->packView->setModel(listModel);

    ui->versionSelectionBox->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->versionSelectionBox->view()->parentWidget()->setMaximumHeight(300);

    m_search_timer.setTimerType(Qt::TimerType::CoarseTimer);
    m_search_timer.setSingleShot(true);

    connect(&m_search_timer, &QTimer::timeout, this, &FlamePage::triggerSearch);

    m_fetch_progress.hideIfInactive(true);
    m_fetch_progress.setFixedHeight(24);
    m_fetch_progress.progressFormat("");

    ui->gridLayout->addWidget(&m_fetch_progress, 2, 0, 1, ui->gridLayout->columnCount());

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

    ui->packView->setItemDelegate(new ProjectItemDelegate(this));
    ui->packDescription->setMetaEntry("FlamePacks");
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
        } else {
            if (m_search_timer.isActive())
                m_search_timer.stop();

            m_search_timer.start(350);
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
    m_fetch_progress.watch(listModel->activeSearchJob().get());
}

void FlamePage::onSelectionChanged(QModelIndex curr, [[maybe_unused]] QModelIndex prev)
{
    ui->versionSelectionBox->clear();

    if (!curr.isValid()) {
        if (isOpened) {
            dialog->setSuggestedPack();
        }
        return;
    }

    current = listModel->data(curr, Qt::UserRole).value<Flame::IndexedPack>();

    if (current.versionsLoaded == false) {
        qDebug() << "Loading flame modpack versions";
        auto netJob = new NetJob(QString("Flame::PackVersions(%1)").arg(current.name), APPLICATION->network());
        auto response = std::make_shared<QByteArray>();
        int addonId = current.addonId;
        netJob->addNetAction(
            Net::ApiDownload::makeByteArray(QString("https://api.curseforge.com/v1/mods/%1/files").arg(addonId), response));

        QObject::connect(netJob, &NetJob::succeeded, this, [this, response, addonId, curr] {
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

            QVariant current_updated;
            current_updated.setValue(current);

            if (!listModel->setData(curr, current_updated, Qt::UserRole))
                qWarning() << "Failed to cache versions for the current pack!";

            // TODO: Check whether it's a connection issue or the project disabled 3rd-party distribution.
            if (current.versionsLoaded && ui->versionSelectionBox->count() < 1) {
                ui->versionSelectionBox->addItem(tr("No version is available!"), -1);
            }
            suggestCurrent();
        });
        QObject::connect(netJob, &NetJob::finished, this, [response, netJob] { netJob->deleteLater(); });
        netJob->start();
    } else {
        for (auto version : current.versions) {
            ui->versionSelectionBox->addItem(version.version, QVariant(version.downloadUrl));
        }

        suggestCurrent();
    }

    // TODO: Check whether it's a connection issue or the project disabled 3rd-party distribution.
    if (current.versionsLoaded && ui->versionSelectionBox->count() < 1) {
        ui->versionSelectionBox->addItem(tr("No version is available!"), -1);
    }

    updateUi();
}

void FlamePage::suggestCurrent()
{
    if (!isOpened) {
        return;
    }

    if (m_selected_version_index == -1) {
        dialog->setSuggestedPack();
        return;
    }

    auto version = current.versions.at(m_selected_version_index);

    QMap<QString, QString> extra_info;
    extra_info.insert("pack_id", QString::number(current.addonId));
    extra_info.insert("pack_version_id", QString::number(version.fileId));

    dialog->setSuggestedPack(current.name, new InstanceImportTask(version.downloadUrl, this, std::move(extra_info)));
    QString editedLogoName;
    editedLogoName = "curseforge_" + current.logoName;
    listModel->getLogo(current.logoName, current.logoUrl,
                       [this, editedLogoName](QString logo) { dialog->setSuggestedIconFromFile(logo, editedLogoName); });
}

void FlamePage::onVersionSelectionChanged(QString version)
{
    bool is_blocked = false;
    ui->versionSelectionBox->currentData().toInt(&is_blocked);

    if (version.isNull() || version.isEmpty() || is_blocked) {
        m_selected_version_index = -1;
        return;
    }

    m_selected_version_index = ui->versionSelectionBox->currentIndex();

    Q_ASSERT(current.versions.at(m_selected_version_index).downloadUrl == ui->versionSelectionBox->currentData().toString());

    suggestCurrent();
}

void FlamePage::updateUi()
{
    QString text = "";
    QString name = current.name;

    if (current.extra.websiteUrl.isEmpty())
        text = name;
    else
        text = "<a href=\"" + current.extra.websiteUrl + "\">" + name + "</a>";
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

    if (current.extraInfoLoaded) {
        if (!current.extra.issuesUrl.isEmpty() || !current.extra.sourceUrl.isEmpty() || !current.extra.wikiUrl.isEmpty()) {
            text += "<br><br>" + tr("External links:") + "<br>";
        }

        if (!current.extra.issuesUrl.isEmpty())
            text += "- " + tr("Issues: <a href=%1>%1</a>").arg(current.extra.issuesUrl) + "<br>";
        if (!current.extra.wikiUrl.isEmpty())
            text += "- " + tr("Wiki: <a href=%1>%1</a>").arg(current.extra.wikiUrl) + "<br>";
        if (!current.extra.sourceUrl.isEmpty())
            text += "- " + tr("Source code: <a href=%1>%1</a>").arg(current.extra.sourceUrl) + "<br>";
    }

    text += "<hr>";
    text += api.getModDescription(current.addonId).toUtf8();

    ui->packDescription->setHtml(text + current.description);
    ui->packDescription->flush();
}
