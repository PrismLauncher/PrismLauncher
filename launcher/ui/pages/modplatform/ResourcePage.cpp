// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only AND Apache-2.0
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
 *  Copyright (c) 2023 Trial97 <alexandru.tripon97@gmail.com>
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

#include "ResourcePage.h"
#include "modplatform/ModIndex.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui_ResourcePage.h"

#include <StringUtils.h>
#include <QDesktopServices>
#include <QKeyEvent>

#include "Markdown.h"

#include "Application.h"
#include "ui/dialogs/ResourceDownloadDialog.h"
#include "ui/pages/modplatform/ResourceModel.h"
#include "ui/widgets/ProjectItem.h"

namespace ResourceDownload {

ResourcePage::ResourcePage(ResourceDownloadDialog* parent, BaseInstance& base_instance)
    : QWidget(parent), m_baseInstance(base_instance), m_ui(new Ui::ResourcePage), m_parentDialog(parent), m_fetchProgress(this, false)
{
    m_ui->setupUi(this);

    m_ui->searchEdit->installEventFilter(this);

    m_ui->versionSelectionBox->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_ui->versionSelectionBox->view()->parentWidget()->setMaximumHeight(300);

    m_searchTimer.setTimerType(Qt::TimerType::CoarseTimer);
    m_searchTimer.setSingleShot(true);

    connect(&m_searchTimer, &QTimer::timeout, this, &ResourcePage::triggerSearch);

    // hide progress bar to prevent weird artifact
    m_fetchProgress.hide();
    m_fetchProgress.hideIfInactive(true);
    m_fetchProgress.setFixedHeight(24);
    m_fetchProgress.progressFormat("");

    m_ui->verticalLayout->insertWidget(1, &m_fetchProgress);

    auto delegate = new ProjectItemDelegate(this);
    m_ui->packView->setItemDelegate(delegate);
    m_ui->packView->installEventFilter(this);
    m_ui->packView->viewport()->installEventFilter(this);

    connect(m_ui->packDescription, &QTextBrowser::anchorClicked, this, &ResourcePage::openUrl);

    connect(m_ui->packView, &QAbstractItemView::doubleClicked, this, &ResourcePage::onResourceToggle);
    connect(delegate, &ProjectItemDelegate::checkboxClicked, this, &ResourcePage::onResourceToggle);
}

ResourcePage::~ResourcePage()
{
    delete m_ui;
    if (m_model)
        delete m_model;
}

void ResourcePage::retranslate()
{
    m_ui->retranslateUi(this);
}

void ResourcePage::openedImpl()
{
    if (!supportsFiltering()) {
        m_ui->resourceFilterButton->setVisible(false);
        m_ui->filterWidget->hide();
    }

    //: String in the search bar of the mod downloading dialog
    m_ui->searchEdit->setPlaceholderText(tr("Search for %1...").arg(resourcesString()));
    m_ui->resourceSelectionButton->setText(tr("Select %1 for download").arg(resourceString()));

    updateSelectionButton();
    triggerSearch();
    m_ui->searchEdit->setFocus();
}

auto ResourcePage::eventFilter(QObject* watched, QEvent* event) -> bool
{
    if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (watched == m_ui->searchEdit) {
            if (keyEvent->key() == Qt::Key_Return) {
                triggerSearch();
                keyEvent->accept();
                return true;
            } else {
                if (m_searchTimer.isActive())
                    m_searchTimer.stop();

                m_searchTimer.start(350);
            }
        } else if (watched == m_ui->packView) {
            // stop the event from going to the confirm button
            if (keyEvent->key() == Qt::Key_Return) {
                onResourceToggle(m_ui->packView->currentIndex());
                keyEvent->accept();
                return true;
            }
        }
    } else if (watched == m_ui->packView->viewport() && event->type() == QEvent::MouseButtonPress) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);

        if (mouseEvent->button() == Qt::MiddleButton) {
            onResourceToggle(m_ui->packView->indexAt(mouseEvent->pos()));
            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}

QString ResourcePage::getSearchTerm() const
{
    return m_ui->searchEdit->text();
}

void ResourcePage::setSearchTerm(QString term)
{
    m_ui->searchEdit->setText(term);
}

void ResourcePage::addSortings()
{
    Q_ASSERT(m_model);

    auto sorts = m_model->getSortingMethods();
    std::sort(sorts.begin(), sorts.end(), [](auto const& l, auto const& r) { return l.index < r.index; });

    for (auto&& sorting : sorts)
        m_ui->sortByBox->addItem(sorting.readable_name, QVariant(sorting.index));
}

bool ResourcePage::setCurrentPack(ModPlatform::IndexedPack::Ptr pack)
{
    QVariant v;
    v.setValue(pack);
    return m_model->setData(m_ui->packView->currentIndex(), v, Qt::UserRole);
}

ModPlatform::IndexedPack::Ptr ResourcePage::getCurrentPack() const
{
    return m_model->data(m_ui->packView->currentIndex(), Qt::UserRole).value<ModPlatform::IndexedPack::Ptr>();
}

void ResourcePage::updateUi(const QModelIndex& index)
{
    if (index != m_ui->packView->currentIndex())
        return;

    auto current_pack = getCurrentPack();
    if (!current_pack) {
        m_ui->packDescription->setHtml({});
        m_ui->packDescription->flush();
        return;
    }
    QString text = "";
    QString name = current_pack->name;

    if (current_pack->websiteUrl.isEmpty())
        text = name;
    else
        text = "<a href=\"" + current_pack->websiteUrl + "\">" + name + "</a>";

    if (!current_pack->authors.empty()) {
        auto authorToStr = [](ModPlatform::ModpackAuthor& author) -> QString {
            if (author.url.isEmpty()) {
                return author.name;
            }
            return QString("<a href=\"%1\">%2</a>").arg(author.url, author.name);
        };
        QStringList authorStrs;
        for (auto& author : current_pack->authors) {
            authorStrs.push_back(authorToStr(author));
        }
        text += "<br>" + tr(" by ") + authorStrs.join(", ");
    }

    if (current_pack->extraDataLoaded) {
        if (current_pack->extraData.status == "archived") {
            text += "<br><br>" + tr("<b>This project has been archived. It will not receive any further updates unless the author decides "
                                    "to unarchive the project.</b>");
        }

        if (!current_pack->extraData.donate.isEmpty()) {
            text += "<br><br>" + tr("Donate information: ");
            auto donateToStr = [](ModPlatform::DonationData& donate) -> QString {
                return QString("<a href=\"%1\">%2</a>").arg(donate.url, donate.platform);
            };
            QStringList donates;
            for (auto& donate : current_pack->extraData.donate) {
                donates.append(donateToStr(donate));
            }
            text += donates.join(", ");
        }

        if (!current_pack->extraData.issuesUrl.isEmpty() || !current_pack->extraData.sourceUrl.isEmpty() ||
            !current_pack->extraData.wikiUrl.isEmpty() || !current_pack->extraData.discordUrl.isEmpty()) {
            text += "<br><br>" + tr("External links:") + "<br>";
        }

        if (!current_pack->extraData.issuesUrl.isEmpty())
            text += "- " + tr("Issues: <a href=%1>%1</a>").arg(current_pack->extraData.issuesUrl) + "<br>";
        if (!current_pack->extraData.wikiUrl.isEmpty())
            text += "- " + tr("Wiki: <a href=%1>%1</a>").arg(current_pack->extraData.wikiUrl) + "<br>";
        if (!current_pack->extraData.sourceUrl.isEmpty())
            text += "- " + tr("Source code: <a href=%1>%1</a>").arg(current_pack->extraData.sourceUrl) + "<br>";
        if (!current_pack->extraData.discordUrl.isEmpty())
            text += "- " + tr("Discord: <a href=%1>%1</a>").arg(current_pack->extraData.discordUrl) + "<br>";
    }

    text += "<hr>";

    m_ui->packDescription->setHtml(StringUtils::htmlListPatch(
        text + (current_pack->extraData.body.isEmpty() ? current_pack->description : markdownToHTML(current_pack->extraData.body))));
    m_ui->packDescription->flush();
}

void ResourcePage::updateSelectionButton()
{
    if (!isOpened || m_selectedVersionIndex < 0) {
        m_ui->resourceSelectionButton->setEnabled(false);
        return;
    }

    m_ui->resourceSelectionButton->setEnabled(true);
    if (auto current_pack = getCurrentPack(); current_pack) {
        if (current_pack->versionsLoaded && current_pack->versions.empty()) {
            m_ui->resourceSelectionButton->setEnabled(false);
            qWarning() << tr("No version available for the selected pack");
        } else if (!current_pack->isVersionSelected(m_selectedVersionIndex))
            m_ui->resourceSelectionButton->setText(tr("Select %1 for download").arg(resourceString()));
        else
            m_ui->resourceSelectionButton->setText(tr("Deselect %1 for download").arg(resourceString()));
    } else {
        qWarning() << "Tried to update the selected button but there is not a pack selected";
    }
}

void ResourcePage::versionListUpdated(const QModelIndex& index)
{
    if (index == m_ui->packView->currentIndex()) {
        auto current_pack = getCurrentPack();

        m_ui->versionSelectionBox->blockSignals(true);
        m_ui->versionSelectionBox->clear();
        m_ui->versionSelectionBox->blockSignals(false);

        if (current_pack) {
            auto installedVersion = m_model->getInstalledPackVersion(current_pack);

            for (int i = 0; i < current_pack->versions.size(); i++) {
                auto& version = current_pack->versions[i];
                if (!m_model->checkVersionFilters(version))
                    continue;

                auto versionText = version.version;
                if (version.version_type.isValid()) {
                    versionText += QString(" [%1]").arg(version.version_type.toString());
                }
                if (version.fileId == installedVersion) {
                    versionText += tr(" [installed]", "Mod version select");
                }

                m_ui->versionSelectionBox->addItem(versionText, QVariant(i));
            }
        }
        if (m_ui->versionSelectionBox->count() == 0) {
            m_ui->versionSelectionBox->addItem(tr("No valid version found."), QVariant(-1));
            m_ui->resourceSelectionButton->setText(tr("Cannot select invalid version :("));
        }

        if (m_enableQueue.contains(index.row())) {
            m_enableQueue.remove(index.row());
            onResourceToggle(index);
        } else
            updateSelectionButton();
    } else if (m_enableQueue.contains(index.row())) {
        m_enableQueue.remove(index.row());
        onResourceToggle(index);
    }
}

void ResourcePage::onSelectionChanged(QModelIndex curr, [[maybe_unused]] QModelIndex prev)
{
    if (!curr.isValid()) {
        return;
    }

    auto current_pack = getCurrentPack();

    bool request_load = false;
    if (!current_pack || !current_pack->versionsLoaded) {
        m_ui->resourceSelectionButton->setText(tr("Loading versions..."));
        m_ui->resourceSelectionButton->setEnabled(false);

        request_load = true;
    } else {
        versionListUpdated(curr);
    }

    if (current_pack && !current_pack->extraDataLoaded)
        request_load = true;

    // we are already requesting this
    if (m_enableQueue.contains(curr.row()))
        request_load = false;

    if (request_load)
        m_model->loadEntry(curr);

    updateUi(curr);
}

void ResourcePage::onVersionSelectionChanged(int index)
{
    m_selectedVersionIndex = m_ui->versionSelectionBox->itemData(index).toInt();
    updateSelectionButton();
}

void ResourcePage::addResourceToDialog(ModPlatform::IndexedPack::Ptr pack, ModPlatform::IndexedVersion& version)
{
    m_parentDialog->addResource(pack, version);
}

void ResourcePage::removeResourceFromDialog(const QString& pack_name)
{
    m_parentDialog->removeResource(pack_name);
}

void ResourcePage::addResourceToPage(ModPlatform::IndexedPack::Ptr pack, ModPlatform::IndexedVersion& ver, ResourceFolderModel* base_model)
{
    bool is_indexed = !APPLICATION->settings()->get("ModMetadataDisabled").toBool();
    m_model->addPack(pack, ver, base_model, is_indexed);
}

void ResourcePage::modelReset()
{
    m_enableQueue.clear();
}

void ResourcePage::removeResourceFromPage(const QString& name)
{
    m_model->removePack(name);
}

void ResourcePage::onResourceSelected()
{
    if (m_selectedVersionIndex < 0)
        return;

    auto current_pack = getCurrentPack();
    if (!current_pack || !current_pack->versionsLoaded || current_pack->versions.size() < m_selectedVersionIndex)
        return;

    auto& version = current_pack->versions[m_selectedVersionIndex];
    Q_ASSERT(!version.downloadUrl.isNull());
    if (version.is_currently_selected)
        removeResourceFromDialog(current_pack->name);
    else
        addResourceToDialog(current_pack, version);

    // Save the modified pack (and prevent warning in release build)
    [[maybe_unused]] bool set = setCurrentPack(current_pack);
    Q_ASSERT(set);

    updateSelectionButton();

    /* Force redraw on the resource list when the selection changes */
    m_ui->packView->repaint();
}

void ResourcePage::onResourceToggle(const QModelIndex& index)
{
    const bool isSelected = index == m_ui->packView->currentIndex();
    auto pack = m_model->data(index, Qt::UserRole).value<ModPlatform::IndexedPack::Ptr>();

    if (pack->versionsLoaded) {
        if (pack->isAnyVersionSelected())
            removeResourceFromDialog(pack->name);
        else {
            auto version = std::find_if(pack->versions.begin(), pack->versions.end(), [this](const ModPlatform::IndexedVersion& version) {
                return m_model->checkVersionFilters(version);
            });

            if (version == pack->versions.end()) {
                auto errorMessage = new QMessageBox(
                    QMessageBox::Warning, tr("No versions available"),
                    tr("No versions for '%1' are available.\nThe author likely blocked third-party launchers.").arg(pack->name),
                    QMessageBox::Ok, this);

                errorMessage->open();
            } else
                addResourceToDialog(pack, *version);
        }

        if (isSelected)
            updateSelectionButton();

        // force update
        QVariant variant;
        variant.setValue(pack);
        m_model->setData(index, variant, Qt::UserRole);
    } else {
        // the model is just 1 dimensional so this is fine
        m_enableQueue.insert(index.row());

        // we can't be sure that this hasn't already been requested...
        // but this does the job well enough and there's not much point preventing edgecases
        if (!isSelected)
            m_model->loadEntry(index);
    }
}

void ResourcePage::openUrl(const QUrl& url)
{
    // do not allow other url schemes for security reasons
    if (!(url.scheme() == "http" || url.scheme() == "https")) {
        qWarning() << "Unsupported scheme" << url.scheme();
        return;
    }

    // detect URLs and search instead

    const QString address = url.host() + url.path();
    QRegularExpressionMatch match;
    QString page;

    auto handlers = urlHandlers();
    for (auto it = handlers.constKeyValueBegin(); it != handlers.constKeyValueEnd(); it++) {
        auto&& [regex, candidate] = *it;
        if (match = QRegularExpression(regex).match(address); match.hasMatch()) {
            page = candidate;
            break;
        }
    }

    if (!page.isNull() && !m_doNotJumpToMod) {
        const QString slug = match.captured(1);

        // ensure the user isn't opening the same mod
        if (auto current_pack = getCurrentPack(); current_pack && slug != current_pack->slug) {
            m_parentDialog->selectPage(page);

            auto newPage = m_parentDialog->selectedPage();

            QLineEdit* searchEdit = newPage->m_ui->searchEdit;
            auto model = newPage->m_model;
            QListView* view = newPage->m_ui->packView;

            auto jump = [url, slug, model, view] {
                for (int row = 0; row < model->rowCount({}); row++) {
                    const QModelIndex index = model->index(row);
                    const auto pack = model->data(index, Qt::UserRole).value<ModPlatform::IndexedPack::Ptr>();

                    if (pack->slug == slug) {
                        view->setCurrentIndex(index);
                        return;
                    }
                }

                // The final fallback.
                QDesktopServices::openUrl(url);
            };

            searchEdit->setText(slug);
            newPage->triggerSearch();

            if (model->hasActiveSearchJob())
                connect(model->activeSearchJob().get(), &Task::finished, jump);
            else
                jump();

            return;
        }
    }

    // open in the user's web browser
    QDesktopServices::openUrl(url);
}

void ResourcePage::openProject(QVariant projectID)
{
    m_ui->sortByBox->hide();
    m_ui->searchEdit->hide();
    m_ui->resourceFilterButton->hide();
    m_ui->packView->hide();
    m_ui->resourceSelectionButton->hide();
    m_doNotJumpToMod = true;

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    auto okBtn = buttonBox->button(QDialogButtonBox::Ok);
    okBtn->setDefault(true);
    okBtn->setAutoDefault(true);
    okBtn->setText(tr("Reinstall"));
    okBtn->setShortcut(tr("Ctrl+Return"));
    okBtn->setEnabled(false);

    auto cancelBtn = buttonBox->button(QDialogButtonBox::Cancel);
    cancelBtn->setDefault(false);
    cancelBtn->setAutoDefault(false);
    cancelBtn->setText(tr("Cancel"));

    connect(okBtn, &QPushButton::clicked, this, [this] {
        onResourceSelected();
        m_parentDialog->accept();
    });

    connect(cancelBtn, &QPushButton::clicked, m_parentDialog, &ResourceDownloadDialog::reject);
    m_ui->gridLayout_4->addWidget(buttonBox, 1, 2);

    connect(m_ui->versionSelectionBox, &QComboBox::currentIndexChanged, this,
            [this, okBtn](int index) { okBtn->setEnabled(m_ui->versionSelectionBox->itemData(index).toInt() >= 0); });

    auto jump = [this] {
        for (int row = 0; row < m_model->rowCount({}); row++) {
            const QModelIndex index = m_model->index(row);
            m_ui->packView->setCurrentIndex(index);
            return;
        }
        m_ui->packDescription->setText(tr("The resource was not found"));
    };

    m_ui->searchEdit->setText("#" + projectID.toString());
    triggerSearch();

    if (m_model->hasActiveSearchJob())
        connect(m_model->activeSearchJob().get(), &Task::finished, jump);
    else
        jump();
}
}  // namespace ResourceDownload
