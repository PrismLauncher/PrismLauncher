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
#include "Version.h"
#include "modplatform/ModIndex.h"
#include "modplatform/ResourceAPI.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/widgets/ModFilterWidget.h"
#include "ui_FlamePage.h"

#include <QKeyEvent>
#include <memory>

#include "FlameModel.h"
#include "InstanceImportTask.h"
#include "StringUtils.h"
#include "modplatform/flame/FlameAPI.h"
#include "ui/dialogs/NewInstanceDialog.h"
#include "ui/widgets/ProjectItem.h"

static FlameAPI api;

FlamePage::FlamePage(NewInstanceDialog* dialog, QWidget* parent)
    : QWidget(parent), m_ui(new Ui::FlamePage), m_dialog(dialog), m_fetch_progress(this, false)
{
    m_ui->setupUi(this);
    m_ui->searchEdit->installEventFilter(this);
    m_listModel = new Flame::ListModel(this);
    m_ui->packView->setModel(m_listModel);

    m_ui->versionSelectionBox->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_ui->versionSelectionBox->view()->parentWidget()->setMaximumHeight(300);

    m_search_timer.setTimerType(Qt::TimerType::CoarseTimer);
    m_search_timer.setSingleShot(true);

    connect(&m_search_timer, &QTimer::timeout, this, &FlamePage::triggerSearch);

    m_fetch_progress.hideIfInactive(true);
    m_fetch_progress.setFixedHeight(24);
    m_fetch_progress.progressFormat("");

    m_ui->verticalLayout->insertWidget(2, &m_fetch_progress);

    // index is used to set the sorting with the curseforge api
    m_ui->sortByBox->addItem(tr("Sort by Featured"));
    m_ui->sortByBox->addItem(tr("Sort by Popularity"));
    m_ui->sortByBox->addItem(tr("Sort by Last Updated"));
    m_ui->sortByBox->addItem(tr("Sort by Name"));
    m_ui->sortByBox->addItem(tr("Sort by Author"));
    m_ui->sortByBox->addItem(tr("Sort by Total Downloads"));

    connect(m_ui->sortByBox, &QComboBox::currentIndexChanged, this, &FlamePage::triggerSearch);
    connect(m_ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &FlamePage::onSelectionChanged);
    connect(m_ui->versionSelectionBox, &QComboBox::currentIndexChanged, this, &FlamePage::onVersionSelectionChanged);

    m_ui->packView->setItemDelegate(new ProjectItemDelegate(this));
    m_ui->packDescription->setMetaEntry("FlamePacks");
    createFilterWidget();
}

FlamePage::~FlamePage()
{
    delete m_ui;
}

bool FlamePage::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_ui->searchEdit && event->type() == QEvent::KeyPress) {
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
    m_ui->retranslateUi(this);
}

void FlamePage::openedImpl()
{
    suggestCurrent();
    triggerSearch();
}

void FlamePage::triggerSearch()
{
    m_ui->packView->selectionModel()->setCurrentIndex({}, QItemSelectionModel::SelectionFlag::ClearAndSelect);
    m_ui->packView->clearSelection();
    m_ui->packDescription->clear();
    m_ui->versionSelectionBox->clear();
    bool filterChanged = m_filterWidget->changed();
    m_listModel->searchWithTerm(m_ui->searchEdit->text(), m_ui->sortByBox->currentIndex(), m_filterWidget->getFilter(), filterChanged);
    m_fetch_progress.watch(m_listModel->activeSearchJob().get());
}

void FlamePage::onSelectionChanged(QModelIndex curr, [[maybe_unused]] QModelIndex prev)
{
    m_ui->versionSelectionBox->clear();

    if (!curr.isValid()) {
        if (isOpened) {
            m_dialog->setSuggestedPack();
        }
        return;
    }

    m_current = m_listModel->data(curr, Qt::UserRole).value<ModPlatform::IndexedPack::Ptr>();

    if (!m_current->versionsLoaded || m_filterWidget->changed()) {
        qDebug() << "Loading flame modpack versions";

        ResourceAPI::Callback<QVector<ModPlatform::IndexedVersion> > callbacks{};

        auto addonId = m_current->addonId;
        // Use default if no callbacks are set
        callbacks.on_succeed = [this, curr, addonId](auto& doc) {
            if (addonId != m_current->addonId) {
                return;  // wrong request
            }

            m_current->versions = doc;
            m_current->versionsLoaded = true;
            auto pred = [this](const ModPlatform::IndexedVersion& v) {
                if (auto filter = m_filterWidget->getFilter())
                    return !filter->checkModpackFilters(v);
                return false;
            };
#if QT_VERSION >= QT_VERSION_CHECK(6, 1, 0)
            m_current->versions.removeIf(pred);
#else
            for (auto it = m_current->versions.begin(); it != m_current->versions.end();)
                if (pred(*it))
                    it = m_current->versions.erase(it);
                else
                    ++it;
#endif
            for (auto version : m_current->versions) {
                m_ui->versionSelectionBox->addItem(version.getVersionDisplayString(), QVariant(version.downloadUrl));
            }

            QVariant current_updated;
            current_updated.setValue(m_current);

            if (!m_listModel->setData(curr, current_updated, Qt::UserRole))
                qWarning() << "Failed to cache versions for the current pack!";

            // TODO: Check whether it's a connection issue or the project disabled 3rd-party distribution.
            if (m_current->versionsLoaded && m_ui->versionSelectionBox->count() < 1) {
                m_ui->versionSelectionBox->addItem(tr("No version is available!"), -1);
            }
            suggestCurrent();
        };
        callbacks.on_fail = [this](QString reason, int) {
            CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->exec();
        };

        auto netJob = api.getProjectVersions({ m_current, {}, {}, ModPlatform::ResourceType::Modpack }, std::move(callbacks));

        m_job = netJob;
        netJob->start();
    } else {
        for (auto version : m_current->versions) {
            m_ui->versionSelectionBox->addItem(version.version, QVariant(version.downloadUrl));
        }

        suggestCurrent();
    }

    // TODO: Check whether it's a connection issue or the project disabled 3rd-party distribution.
    if (m_current->versionsLoaded && m_ui->versionSelectionBox->count() < 1) {
        m_ui->versionSelectionBox->addItem(tr("No version is available!"), -1);
    }

    updateUi();
}

void FlamePage::suggestCurrent()
{
    if (!isOpened) {
        return;
    }

    if (m_selected_version_index == -1) {
        m_dialog->setSuggestedPack();
        return;
    }

    auto version = m_current->versions.at(m_selected_version_index);

    QMap<QString, QString> extra_info;
    extra_info.insert("pack_id", m_current->addonId.toString());
    extra_info.insert("pack_version_id", version.fileId.toString());

    m_dialog->setSuggestedPack(m_current->name, new InstanceImportTask(version.downloadUrl, this, std::move(extra_info)));
    QString editedLogoName = "curseforge_" + m_current->logoName;
    m_listModel->getLogo(m_current->logoName, m_current->logoUrl,
                         [this, editedLogoName](QString logo) { m_dialog->setSuggestedIconFromFile(logo, editedLogoName); });
}

void FlamePage::onVersionSelectionChanged(int index)
{
    bool is_blocked = false;
    m_ui->versionSelectionBox->itemData(index).toInt(&is_blocked);

    if (index == -1 || is_blocked) {
        m_selected_version_index = -1;
        return;
    }

    m_selected_version_index = index;

    Q_ASSERT(m_current->versions.at(m_selected_version_index).downloadUrl == m_ui->versionSelectionBox->currentData().toString());

    suggestCurrent();
}

void FlamePage::updateUi()
{
    QString text = "";
    QString name = m_current->name;

    if (m_current->websiteUrl.isEmpty())
        text = name;
    else
        text = "<a href=\"" + m_current->websiteUrl + "\">" + name + "</a>";
    if (!m_current->authors.empty()) {
        auto authorToStr = [](ModPlatform::ModpackAuthor& author) {
            if (author.url.isEmpty()) {
                return author.name;
            }
            return QString("<a href=\"%1\">%2</a>").arg(author.url, author.name);
        };
        QStringList authorStrs;
        for (auto& author : m_current->authors) {
            authorStrs.push_back(authorToStr(author));
        }
        text += "<br>" + tr(" by ") + authorStrs.join(", ");
    }

    if (m_current->extraDataLoaded) {
        if (!m_current->extraData.issuesUrl.isEmpty() || !m_current->extraData.sourceUrl.isEmpty() ||
            !m_current->extraData.wikiUrl.isEmpty()) {
            text += "<br><br>" + tr("External links:") + "<br>";
        }

        if (!m_current->extraData.issuesUrl.isEmpty())
            text += "- " + tr("Issues: <a href=%1>%1</a>").arg(m_current->extraData.issuesUrl) + "<br>";
        if (!m_current->extraData.wikiUrl.isEmpty())
            text += "- " + tr("Wiki: <a href=%1>%1</a>").arg(m_current->extraData.wikiUrl) + "<br>";
        if (!m_current->extraData.sourceUrl.isEmpty())
            text += "- " + tr("Source code: <a href=%1>%1</a>").arg(m_current->extraData.sourceUrl) + "<br>";
    }

    text += "<hr>";
    text += api.getModDescription(m_current->addonId.toInt()).toUtf8();

    m_ui->packDescription->setHtml(StringUtils::htmlListPatch(text + m_current->description));
    m_ui->packDescription->flush();
}
QString FlamePage::getSerachTerm() const
{
    return m_ui->searchEdit->text();
}

void FlamePage::setSearchTerm(QString term)
{
    m_ui->searchEdit->setText(term);
}

void FlamePage::createFilterWidget()
{
    auto widget = ModFilterWidget::create(nullptr, false);
    m_filterWidget.swap(widget);
    auto old = m_ui->splitter->replaceWidget(0, m_filterWidget.get());
    // because we replaced the widget we also need to delete it
    if (old) {
        delete old;
    }

    connect(m_ui->filterButton, &QPushButton::clicked, this, [this] { m_filterWidget->setHidden(!m_filterWidget->isHidden()); });

    connect(m_filterWidget.get(), &ModFilterWidget::filterChanged, this, &FlamePage::triggerSearch);
    auto response = std::make_shared<QByteArray>();
    m_categoriesTask = FlameAPI::getCategories(response.get(), ModPlatform::ResourceType::Modpack);
    connect(m_categoriesTask.get(), &Task::succeeded, [this, response]() {
        auto categories = FlameAPI::loadModCategories(response.get());
        m_filterWidget->setCategories(categories);
    });
    m_categoriesTask->start();
}
