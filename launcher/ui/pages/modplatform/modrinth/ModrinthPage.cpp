// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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
 *      Copyright 2021-2022 kb1000
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

#include "ModrinthPage.h"
#include "Version.h"
#include "modplatform/ModIndex.h"
#include "modplatform/modrinth/ModrinthAPI.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui_ModrinthPage.h"

#include "ModrinthModel.h"

#include "BuildConfig.h"
#include "InstanceImportTask.h"
#include "Json.h"
#include "Markdown.h"
#include "StringUtils.h"

#include "ui/widgets/ProjectItem.h"

#include "net/ApiDownload.h"

#include <QComboBox>
#include <QKeyEvent>
#include <QPushButton>

ModrinthPage::ModrinthPage(NewInstanceDialog* dialog, QWidget* parent)
    : QWidget(parent), m_ui(new Ui::ModrinthPage), m_dialog(dialog), m_fetch_progress(this, false)
{
    m_ui->setupUi(this);
    createFilterWidget();

    m_ui->searchEdit->installEventFilter(this);
    m_model = new Modrinth::ModpackListModel(this);
    m_ui->packView->setModel(m_model);

    m_ui->versionSelectionBox->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_ui->versionSelectionBox->view()->parentWidget()->setMaximumHeight(300);

    m_search_timer.setTimerType(Qt::TimerType::CoarseTimer);
    m_search_timer.setSingleShot(true);

    connect(&m_search_timer, &QTimer::timeout, this, &ModrinthPage::triggerSearch);

    m_fetch_progress.hideIfInactive(true);
    m_fetch_progress.setFixedHeight(24);
    m_fetch_progress.progressFormat("");

    m_ui->verticalLayout->insertWidget(1, &m_fetch_progress);

    m_ui->sortByBox->addItem(tr("Sort by Relevance"));
    m_ui->sortByBox->addItem(tr("Sort by Total Downloads"));
    m_ui->sortByBox->addItem(tr("Sort by Follows"));
    m_ui->sortByBox->addItem(tr("Sort by Newest"));
    m_ui->sortByBox->addItem(tr("Sort by Last Updated"));

    connect(m_ui->sortByBox, &QComboBox::currentIndexChanged, this, &ModrinthPage::triggerSearch);
    connect(m_ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &ModrinthPage::onSelectionChanged);
    connect(m_ui->versionSelectionBox, &QComboBox::currentIndexChanged, this, &ModrinthPage::onVersionSelectionChanged);

    m_ui->packView->setItemDelegate(new ProjectItemDelegate(this));
    m_ui->packDescription->setMetaEntry(metaEntryBase());
}

ModrinthPage::~ModrinthPage()
{
    delete m_ui;
}

void ModrinthPage::retranslate()
{
    m_ui->retranslateUi(this);
}

void ModrinthPage::openedImpl()
{
    BasePage::openedImpl();
    suggestCurrent();
    triggerSearch();
}

bool ModrinthPage::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_ui->searchEdit && event->type() == QEvent::KeyPress) {
        auto* keyEvent = reinterpret_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return) {
            this->triggerSearch();
            keyEvent->accept();
            return true;
        } else {
            if (m_search_timer.isActive())
                m_search_timer.stop();

            m_search_timer.start(350);
        }
    }
    return QObject::eventFilter(watched, event);
}

void ModrinthPage::onSelectionChanged(QModelIndex curr, [[maybe_unused]] QModelIndex prev)
{
    m_ui->versionSelectionBox->clear();

    if (!curr.isValid()) {
        if (isOpened) {
            m_dialog->setSuggestedPack();
        }
        return;
    }

    m_current = m_model->data(curr, Qt::UserRole).value<ModPlatform::IndexedPack::Ptr>();
    auto name = m_current->name;

    if (!m_current->extraDataLoaded) {
        qDebug() << "Loading modrinth modpack information";
        ResourceAPI::Callback<ModPlatform::IndexedPack::Ptr> callbacks;

        auto id = m_current->addonId;
        callbacks.on_fail = [this](QString reason, int) {
            CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->exec();
        };
        callbacks.on_succeed = [this, id, curr](auto& pack) {
            if (id != m_current->addonId) {
                return;  // wrong request?
            }

            QVariant current_updated;
            current_updated.setValue(pack);

            if (!m_model->setData(curr, current_updated, Qt::UserRole))
                qWarning() << "Failed to cache extra info for the current pack!";

            suggestCurrent();
            updateUI();
        };
        if (auto netJob = m_api.getProjectInfo({ m_current }, std::move(callbacks)); netJob) {
            m_job = netJob;
            m_job->start();
        }

    } else
        updateUI();

    if (!m_current->versionsLoaded || m_filterWidget->changed()) {
        qDebug() << "Loading modrinth modpack versions";

        ResourceAPI::Callback<QVector<ModPlatform::IndexedVersion>> callbacks{};

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
            for (const auto& version : m_current->versions) {
                m_ui->versionSelectionBox->addItem(version.getVersionDisplayString(), QVariant(version.fileId));
            }

            QVariant current_updated;
            current_updated.setValue(m_current);

            if (!m_model->setData(curr, current_updated, Qt::UserRole))
                qWarning() << "Failed to cache versions for the current pack!";

            suggestCurrent();
        };
        callbacks.on_fail = [this](QString reason, int) {
            CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->exec();
        };

        auto netJob = m_api.getProjectVersions({ m_current, {}, {}, ModPlatform::ResourceType::Modpack }, std::move(callbacks));

        m_job2 = netJob;
        m_job2->start();

    } else {
        for (auto version : m_current->versions) {
            if (!version.version.contains(version.version))
                m_ui->versionSelectionBox->addItem(QString("%1 - %2").arg(version.version, version.version_number),
                                                   QVariant(version.fileId));
            else
                m_ui->versionSelectionBox->addItem(version.version, QVariant(version.fileId));
        }

        suggestCurrent();
    }
}

void ModrinthPage::updateUI()
{
    QString text = "";

    if (m_current->websiteUrl.isEmpty())
        text = m_current->name;
    else
        text = "<a href=\"" + m_current->websiteUrl + "\">" + m_current->name + "</a>";

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
        if (m_current->extraData.status == "archived") {
            text += "<br><br>" + tr("<b>This project has been archived. It will not receive any further updates unless the author decides "
                                    "to unarchive the project.</b>");
        }

        if (!m_current->extraData.donate.isEmpty()) {
            text += "<br><br>" + tr("Donate information: ");
            auto donateToStr = [](ModPlatform::DonationData& donate) -> QString {
                return QString("<a href=\"%1\">%2</a>").arg(donate.url, donate.platform);
            };
            QStringList donates;
            for (auto& donate : m_current->extraData.donate) {
                donates.append(donateToStr(donate));
            }
            text += donates.join(", ");
        }

        if (!m_current->extraData.issuesUrl.isEmpty() || !m_current->extraData.sourceUrl.isEmpty() ||
            !m_current->extraData.wikiUrl.isEmpty() || !m_current->extraData.discordUrl.isEmpty()) {
            text += "<br><br>" + tr("External links:") + "<br>";
        }

        if (!m_current->extraData.issuesUrl.isEmpty())
            text += "- " + tr("Issues: <a href=%1>%1</a>").arg(m_current->extraData.issuesUrl) + "<br>";
        if (!m_current->extraData.wikiUrl.isEmpty())
            text += "- " + tr("Wiki: <a href=%1>%1</a>").arg(m_current->extraData.wikiUrl) + "<br>";
        if (!m_current->extraData.sourceUrl.isEmpty())
            text += "- " + tr("Source code: <a href=%1>%1</a>").arg(m_current->extraData.sourceUrl) + "<br>";
        if (!m_current->extraData.discordUrl.isEmpty())
            text += "- " + tr("Discord: <a href=%1>%1</a>").arg(m_current->extraData.discordUrl) + "<br>";
    }

    text += "<hr>";

    text += markdownToHTML(m_current->extraData.body.toUtf8());

    m_ui->packDescription->setHtml(StringUtils::htmlListPatch(text + m_current->description));
    m_ui->packDescription->flush();
}

void ModrinthPage::suggestCurrent()
{
    if (!isOpened) {
        return;
    }

    if (m_selectedVersion.isEmpty()) {
        m_dialog->setSuggestedPack();
        return;
    }

    for (auto& ver : m_current->versions) {
        if (ver.fileId == m_selectedVersion) {
            QMap<QString, QString> extra_info;
            extra_info.insert("pack_id", m_current->addonId.toString());
            extra_info.insert("pack_version_id", ver.fileId.toString());

            m_dialog->setSuggestedPack(m_current->name, ver.version, new InstanceImportTask(ver.downloadUrl, this, std::move(extra_info)));
            QString editedLogoName = "modrinth_" + m_current->logoName;
            m_model->getLogo(m_current->logoName, m_current->logoUrl,
                             [this, editedLogoName](QString logo) { m_dialog->setSuggestedIconFromFile(logo, editedLogoName); });

            break;
        }
    }
}

void ModrinthPage::triggerSearch()
{
    m_ui->packView->selectionModel()->setCurrentIndex({}, QItemSelectionModel::SelectionFlag::ClearAndSelect);
    m_ui->packView->clearSelection();
    m_ui->packDescription->clear();
    m_ui->versionSelectionBox->clear();
    bool filterChanged = m_filterWidget->changed();
    m_model->searchWithTerm(m_ui->searchEdit->text(), m_ui->sortByBox->currentIndex(), m_filterWidget->getFilter(), filterChanged);
    m_fetch_progress.watch(m_model->activeSearchJob().get());
}

void ModrinthPage::onVersionSelectionChanged(int index)
{
    if (index == -1) {
        m_selectedVersion = "";
        return;
    }
    m_selectedVersion = m_ui->versionSelectionBox->itemData(index).toString();
    suggestCurrent();
}

void ModrinthPage::setSearchTerm(QString term)
{
    m_ui->searchEdit->setText(term);
}

QString ModrinthPage::getSerachTerm() const
{
    return m_ui->searchEdit->text();
}

void ModrinthPage::createFilterWidget()
{
    auto widget = ModFilterWidget::create(nullptr, true);
    m_filterWidget.swap(widget);
    auto old = m_ui->splitter->replaceWidget(0, m_filterWidget.get());
    // because we replaced the widget we also need to delete it
    if (old) {
        delete old;
    }

    connect(m_ui->filterButton, &QPushButton::clicked, this, [this] { m_filterWidget->setHidden(!m_filterWidget->isHidden()); });

    connect(m_filterWidget.get(), &ModFilterWidget::filterChanged, this, &ModrinthPage::triggerSearch);
    auto response = std::make_shared<QByteArray>();
    m_categoriesTask = ModrinthAPI::getModCategories(response.get());
    connect(m_categoriesTask.get(), &Task::succeeded, [this, response]() {
        auto categories = ModrinthAPI::loadCategories(response.get(), "modpack");
        m_filterWidget->setCategories(categories);
    });
    m_categoriesTask->start();
}
