// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2022 TheKodeToad <TheKodeToad@proton.me>
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

#include "ModFolderPage.h"
#include "ProfileCheckStateDelegate.h"
#include "ProfileOverviewWidget.h"
#include "minecraft/mod/Resource.h"
#include "ui/dialogs/ExportToModListDialog.h"
#include "ui/dialogs/InstallLoaderDialog.h"
#include "ui_ExternalResourcesPage.h"

#include <QAbstractItemModel>
#include <QAction>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QHideEvent>
#include <QMenu>
#include <QShowEvent>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QInputDialog>
#include <QBoxLayout>
#include <QGridLayout>
#include <QStackedWidget>
#include <algorithm>
#include <memory>

#include "Application.h"

#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ResourceDownloadDialog.h"
#include "ui/dialogs/ResourceUpdateDialog.h"

#include "minecraft/PackProfile.h"
#include "minecraft/VersionFilterData.h"
#include "minecraft/mod/Mod.h"
#include "minecraft/mod/ModFolderModel.h"

#include "tasks/ConcurrentTask.h"
#include "tasks/Task.h"
#include "ui/dialogs/ProgressDialog.h"

ModFolderPage::~ModFolderPage()
{
    if (m_filterWindow) {
        m_filterWindow->removeEventFilter(this);
    }
    if (ui && ui->treeView && ui->treeView->viewport()) {
        ui->treeView->viewport()->removeEventFilter(this);
    }
}

void ModFolderPage::repaintActiveColumn()
{
    if (ui && ui->treeView && ui->treeView->viewport())
        ui->treeView->viewport()->update();
}

void ModFolderPage::refreshOverviewIfActive()
{
    if (m_actionProfileOverview && m_actionProfileOverview->isChecked())
        refreshOverview();
}

QString ModFolderPage::extraHeaderInfoString()
{
    if (m_actionProfileOverview && m_actionProfileOverview->isChecked()) {
        QStringList profileNames;
        for (int i = 0; i < m_profileTabBar->count(); ++i)
            profileNames.append(m_profileTabBar->tabText(i));

        const bool overviewDefault = loadOverviewDefault();
        const QStringList selected = loadRuntimeSelection();

        QSet<QString> overviewIds;
        if (overviewDefault) {
            if (!profileNames.isEmpty())
                overviewIds = m_profileStates.value(profileNames.at(0));
        } else {
            for (const QString& pname : selected)
                overviewIds += m_profileStates.value(pname);
        }

        const int installedCount = m_model->size();
        const int enabledCount   = static_cast<int>(overviewIds.size());

        if (enabledCount != 0)
            return tr(" (%1 installed, %2 enabled)").arg(installedCount).arg(enabledCount);
        return tr(" (%1 installed)").arg(installedCount);
    }

    return ExternalResourcesPage::extraHeaderInfoString();
}

ModFolderPage::ModFolderPage(MinecraftInstance* inst, ModFolderModel* model, QWidget* parent)
    : ExternalResourcesPage(inst, model, parent), m_model(model)
{
    disconnect(ui->treeView, &ModListView::activated, this, nullptr);
    connect(ui->treeView, &ModListView::activated,
            this, &ModFolderPage::onModItemActivated);

    m_profileTabBar = new QTabBar(this);
    m_profileTabBar->setExpanding(false);
    m_profileTabBar->setDrawBase(false);
    m_profileTabBar->setDocumentMode(false);
    m_profileTabBar->setUsesScrollButtons(m_profileTabBar->count() > 1);
    m_profileTabBar->setElideMode(Qt::ElideNone);
    m_profileTabBar->setContextMenuPolicy(Qt::CustomContextMenu);
    m_profileTabBar->setStyleSheet(
        "QTabBar::tab {"
        "  padding: 4px 12px;"
        "  border: none;"
        "  background: palette(button);"
        "  color: palette(button-text);"
        "  margin-right: 1px;"
        "}"
        "QTabBar::tab:first {"
        "  font-weight: bold;"
        "  margin-right: 8px;"
        "}"
        "QTabBar::tab:selected {"
        "  background: palette(base);"
        "  color: palette(text);"
        "  border-bottom: 2px solid palette(highlight);"
        "}"
        "QTabBar::tab:hover:!selected {"
        "  background: palette(alternate-base);"
        "  color: palette(button-text);"
        "}");
    m_newTabButton = new QToolButton(this);
    m_newTabButton->setText(QStringLiteral("+"));
    m_newTabButton->setToolTip(tr("New Tab"));
    m_newTabButton->setAutoRaise(true);
    m_newTabButton->setFixedSize(24, 24);

    if (ui->treeView->parentWidget() && ui->treeView->parentWidget()->layout()) {
        QLayout* layout = ui->treeView->parentWidget()->layout();
        if (auto* grid = qobject_cast<QGridLayout*>(layout)) {

            m_tabRowContainer = new QWidget(this);
            auto* tabRowLayout = new QHBoxLayout(m_tabRowContainer);
            tabRowLayout->setContentsMargins(0, 0, 0, 0);
            tabRowLayout->setSpacing(0);
            tabRowLayout->addWidget(m_profileTabBar, 0);
            tabRowLayout->addWidget(m_newTabButton,  0);
            tabRowLayout->addStretch(1);
            grid->addWidget(m_tabRowContainer, 0, 1, 1, 2);

            grid->removeWidget(ui->treeView);
            m_contentStack = new QStackedWidget(this);
            m_contentStack->addWidget(ui->treeView);
            m_overviewWidget = new ProfileOverviewWidget(this);
            m_contentStack->addWidget(m_overviewWidget);
            m_contentStack->setCurrentIndex(0);
            grid->addWidget(m_contentStack, 1, 1, 1, 2);
        }
    }

    auto* delegate = new ProfileCheckStateDelegate(
        &m_profileStates, &m_currentProfile,
        qobject_cast<QSortFilterProxyModel*>(m_filterModel),
        m_model, this);
    ui->treeView->setItemDelegateForColumn(ModFolderModel::ActiveColumn, delegate);
    connect(delegate, &ProfileCheckStateDelegate::membershipToggled,
            this, &ModFolderPage::onDelegateMembershipToggled);

    ui->treeView->viewport()->installEventFilter(this);
    m_profileTabBar->installEventFilter(this);
    connect(m_profileTabBar, &QTabBar::currentChanged, this, [this](int index) {
        if (m_applyingProfile) {
            return;
        }
        ++m_profileSwitchGeneration;
        applyProfileSwitch(index, m_profileSwitchGeneration);
    });
    connect(m_profileTabBar, &QTabBar::customContextMenuRequested,
            this, &ModFolderPage::onTabContextMenuRequested);
    connect(m_newTabButton, &QToolButton::clicked, this, [this] {
        onTabNewToRight(m_profileTabBar->count() - 1);
    });

    connect(ui->filterEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        if (m_overviewWidget)
            m_overviewWidget->filterTextChanged(text);
    });

    ui->actionDownloadItem->setText(tr("Download Mods"));
    ui->actionDownloadItem->setToolTip(tr("Download mods from online mod platforms"));
    ui->actionDownloadItem->setEnabled(true);
    ui->actionsToolbar->insertActionBefore(ui->actionAddItem, ui->actionDownloadItem);

    ui->actionsToolbar->setIconSize(QSize(16, 16));
    m_actionProfileOverview = new QAction(tr("Profile Overview"), this);
    m_actionProfileOverview->setCheckable(true);
    m_actionProfileOverview->setIcon(QIcon::fromTheme("loadermods"));
    m_actionProfileOverview->setToolTip(
        tr("Show the read-only launch composition overview.\n"
           "Select which profiles are combined when launching the game."));
    ui->actionsToolbar->insertActionBefore(ui->actionDownloadItem, m_actionProfileOverview);
    ui->actionsToolbar->insertSeparator(ui->actionDownloadItem);
    ui->actionsToolbar->setContextMenuPolicy(Qt::PreventContextMenu);

    for (auto* barAction : ui->actionsToolbar->actions()) {
        QWidget* wid = ui->actionsToolbar->widgetForAction(barAction);
        if (auto* btn = qobject_cast<QToolButton*>(wid)) {
            if (btn->text() == m_actionProfileOverview->text()) {
                QObject::disconnect(ui->actionsToolbar, &QToolBar::iconSizeChanged,
                                    btn, &QToolButton::setIconSize);
                btn->setIconSize(QSize(16, 16));
                btn->setStyleSheet("QToolButton { text-align: left; spacing: 4px; padding-left: 6px; }");
                break;
            }
        }
    }

    if (m_actionProfileOverview && m_contentStack) {
        connect(m_actionProfileOverview, &QAction::toggled, this, [this](bool checked) {
            if (m_contentStack)
                m_contentStack->setCurrentIndex(checked ? 1 : 0);
            if (m_tabRowContainer)
                m_tabRowContainer->setVisible(!checked);
            if (ui && ui->frame)
                ui->frame->setVisible(!checked);

            if (ui && ui->actionsToolbar)
                ui->actionsToolbar->setVisible(!checked);

            if (m_overviewWidget)
                m_overviewWidget->setOverviewActive(checked);

            if (checked) {
                refreshOverview();
            } else {
                if (updateExtraInfo)
                    updateExtraInfo(id(), extraHeaderInfoString());
            }
        });
    }

    connect(ui->actionDownloadItem, &QAction::triggered, this, &ModFolderPage::downloadMods);

    ui->actionUpdateItem->setToolTip(tr("Try to check or update all selected mods (all mods if none are selected)"));
    connect(ui->actionUpdateItem, &QAction::triggered, this, &ModFolderPage::updateMods);
    ui->actionsToolbar->insertActionBefore(ui->actionAddItem, ui->actionUpdateItem);

    auto* updateMenu = new QMenu(this);

    auto* update = updateMenu->addAction(tr("Check for Updates"));
    connect(update, &QAction::triggered, this, &ModFolderPage::updateMods);

    updateMenu->addAction(ui->actionVerifyItemDependencies);
    connect(ui->actionVerifyItemDependencies, &QAction::triggered, this, [this] { updateMods(true); });

    auto depsDisabled = APPLICATION->settings()->getSetting("ModDependenciesDisabled");
    ui->actionVerifyItemDependencies->setVisible(!depsDisabled->get().toBool());
    connect(depsDisabled.get(), &Setting::SettingChanged, this,
            [this](const Setting&, const QVariant& value) { ui->actionVerifyItemDependencies->setVisible(!value.toBool()); });

    updateMenu->addAction(ui->actionResetItemMetadata);
    connect(ui->actionResetItemMetadata, &QAction::triggered, this, &ModFolderPage::deleteModMetadata);

    ui->actionUpdateItem->setMenu(updateMenu);

    ui->actionChangeVersion->setToolTip(tr("Change a mod's version."));
    connect(ui->actionChangeVersion, &QAction::triggered, this, &ModFolderPage::changeModVersion);
    ui->actionsToolbar->insertActionAfter(ui->actionUpdateItem, ui->actionChangeVersion);

    ui->actionViewHomepage->setToolTip(tr("View the homepages of all selected mods."));

    ui->actionExportMetadata->setToolTip(tr("Export mod's metadata to text."));
    connect(ui->actionExportMetadata, &QAction::triggered, this, &ModFolderPage::exportModMetadata);
    ui->actionsToolbar->insertActionAfter(ui->actionViewHomepage, ui->actionExportMetadata);

    ui->actionsToolbar->insertActionAfter(ui->actionViewFolder, ui->actionViewConfigs);

    m_settingsPrefix = m_model->dir().dirName();
    m_instance->settings()->getOrRegisterSetting(profileListKey(), QStringList() << "Default");
    m_instance->settings()->getOrRegisterSetting(lastActiveIndexKey(), 0);
    m_instance->settings()->getOrRegisterSetting(lastActiveNameKey(), QString());
    m_instance->settings()->getOrRegisterSetting(runtimeProfilesKey(), QStringList());
    m_instance->settings()->getOrRegisterSetting(overviewDefaultKey(), true);

    QStringList profileList = m_instance->settings()->get(profileListKey()).toStringList();
    if (profileList.isEmpty()) {
        profileList.append("Default");
    }
    m_profileTabBar->blockSignals(true);
    for (const QString& name : profileList) {
        m_profileTabBar->addTab(name);
        QString key = profileKey(name);
        m_instance->settings()->getOrRegisterSetting(key, QStringList());
        QStringList saved = m_instance->settings()->get(key).toStringList();
        m_profileStates[name] = QSet<QString>(saved.begin(), saved.end());
    }
    m_profileTabBar->blockSignals(false);
    m_profileTabBar->setUsesScrollButtons(m_profileTabBar->count() > 1);

    int savedIndex = m_instance->settings()->get(lastActiveIndexKey()).toInt();
    if (savedIndex < 0 || savedIndex >= m_profileTabBar->count()) {
        savedIndex = 0;
    }
    QString savedName = m_instance->settings()->get(lastActiveNameKey()).toString();
    if (!savedName.isEmpty()) {
        for (int i = 0; i < m_profileTabBar->count(); ++i) {
            if (m_profileTabBar->tabText(i) == savedName) {
                savedIndex = i;
                break;
            }
        }
    }
    m_profileTabBar->blockSignals(true);
    m_profileTabBar->setCurrentIndex(savedIndex);
    m_profileTabBar->blockSignals(false);

    applyProfileSwitch(m_profileTabBar->currentIndex(), m_profileSwitchGeneration, /*isInitialLoad=*/true);

    connect(m_instance, &BaseInstance::runningStatusChanged, this, [this](bool running) {
        if (!running) {
            refreshOverviewIfActive();
            if (m_overviewWidget)
                m_overviewWidget->setRunningLocked(false);
        } else {
            if (m_overviewWidget)
                m_overviewWidget->setRunningLocked(true);
            refreshOverviewIfActive();
        }
    });

    connect(m_overviewWidget, &ProfileOverviewWidget::defaultSelected,
            this, &ModFolderPage::onOverviewDefaultSelected);
    connect(m_overviewWidget, &ProfileOverviewWidget::profileSelectionToggled,
            this, &ModFolderPage::onOverviewProfileSelectionToggled);
    connect(m_overviewWidget, &ProfileOverviewWidget::overviewExitRequested,
            this, [this] {
                if (m_actionProfileOverview)
                    m_actionProfileOverview->setChecked(false);
            });

    connect(m_model, &ModFolderModel::updateFinished, this, [this] {
        refreshOverviewIfActive();
    });
    // mod_id() is only stable after all LocalModParseTask instances finish — until then it returns a filename-based fallback rather than the declared metadata ID.
    // Seeding updates only the in-memory profile state; it does not modify settings or the filesystem.
    {
        auto seeded = std::make_shared<bool>(false);

        auto doSeed = [this, seeded]() {
            if (*seeded)
                return;
            if (m_model->rowCount() == 0 || m_model->hasPendingParseTasks())
                return;
            for (int i = 0; i < m_model->rowCount(); ++i) {
                if (!m_model->at(i).isResolved())
                    return;
            }
            *seeded = true;

            auto* settings = m_instance->settings();
            bool needsRepaint = false;
            for (auto it = m_profileStates.begin(); it != m_profileStates.end(); ++it) {
                const QString& name = it.key();
                if (settings->containsValue(profileKey(name))) {
                    continue;
                }
                QSet<QString> fromDisk;
                for (int i = 0; i < m_model->rowCount(); ++i) {
                    const Mod& mod = m_model->at(i);
                    if (mod.enabled())
                        fromDisk.insert(mod.mod_id());
                }
                it.value() = fromDisk;
                needsRepaint = true;
            }
            if (needsRepaint)
                repaintActiveColumn();
        };

        connect(m_model, &ModFolderModel::updateFinished,     this, doSeed, Qt::SingleShotConnection);
        connect(m_model, &ResourceFolderModel::parseFinished, this, doSeed);
    }
    connect(m_model, &ModFolderModel::dataChanged, this, [this] {
        refreshOverviewIfActive();
    });
}

bool ModFolderPage::shouldDisplay() const
{
    return true;
}

void ModFolderPage::updateFrame(const QModelIndex& current, [[maybe_unused]] const QModelIndex& previous)
{
    auto sourceCurrent = m_filterModel->mapToSource(current);
    int row = sourceCurrent.row();
    const Mod& mod = m_model->at(row);
    ui->frame->updateWithMod(mod);
}

void ModFolderPage::removeItems(const QItemSelection& selection)
{
    if (m_instance != nullptr && m_instance->isRunning()) {
        auto response = CustomMessageBox::selectable(this, tr("Confirm Delete"),
                                                     tr("If you remove mods while the game is running it may crash your game.\n"
                                                        "Are you sure you want to do this?"),
                                                     QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
                            ->exec();

        if (response != QMessageBox::Yes) {
            return;
        }
    }

    auto indexes = selection.indexes();
    auto affected = m_model->getAffectedMods(indexes, EnableAction::DISABLE);
    if (!affected.isEmpty()) {
        auto* box = CustomMessageBox::selectable(this, tr("Confirm Disable"),
                                                 tr("The mods you are trying to delete are required by %1 mods.\n"
                                                    "Do you want to disable them?")
                                                     .arg(affected.length()),
                                                 QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                                                 QMessageBox::Cancel);
        QString details = tr("The following mods depend on the mod(s) you want to remove:");
        for (auto indx : affected) {
            const Mod& mod = m_model->at(indx.row());
            details += QString("\n- %1 (%2)").arg(mod.name(), mod.internalId());
        }
        box->setDetailedText(details);
        const auto response = box->exec();

        if (response == QMessageBox::Cancel) {
            return;
        }
        if (response == QMessageBox::Yes) {
            m_model->setResourceEnabled(affected, EnableAction::DISABLE);
        }
    }
    m_model->deleteResources(indexes);
}

void ModFolderPage::downloadMods()
{
    auto* profile = m_instance->getPackProfile();
    if (!profile->getModLoaders().has_value() && handleNoModLoader()) {
        return;
    }

    m_downloadDialog = ResourceDownload::ResourceDownloadDialog::createMod(this, m_model, m_instance);
    connect(this, &QObject::destroyed, m_downloadDialog, &QDialog::close);
    connect(m_downloadDialog, &QDialog::finished, this, &ModFolderPage::downloadDialogFinished);

    m_downloadDialog->open();
}

void ModFolderPage::downloadDialogFinished(int result)
{
    if (m_downloadFlowActive) {
        return;
    }
    m_downloadFlowActive = true;
    auto dialog = m_downloadDialog;
    m_downloadDialog.clear();
    if (result != 0) {
        ConcurrentTask tasks(tr("Download Mods"), APPLICATION->settings()->get("NumberOfConcurrentDownloads").toInt());
        connect(&tasks, &Task::failed, this, [this](const QString& reason) {
            CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->show();
        });
        connect(&tasks, &Task::succeeded, this, [this, &tasks]() {
            QStringList warnings = tasks.warnings();
            if (warnings.count()) {
                CustomMessageBox::selectable(this, tr("Warnings"), warnings.join('\n'), QMessageBox::Warning)->show();
            }
        });

        if (dialog) {
            for (auto& task : dialog->getTasks()) {
                tasks.addTask(task);
            }
        } else {
            qWarning() << "ResourceDownloadDialog vanished before we could collect tasks!";
        }

        ProgressDialog loadDialog(this);
        loadDialog.setSkipButton(true, tr("Abort"));
        loadDialog.execWithTask(&tasks);

        const QString capturedProfile = m_currentProfile;
        const int capturedGeneration  = m_profileSwitchGeneration;

        QSet<QString> knownModIds;
        for (int i = 0; i < m_model->rowCount(); ++i)
            knownModIds.insert(m_model->at(i).mod_id());

        if (!capturedProfile.isEmpty()) {
            auto parseConn = std::make_shared<QMetaObject::Connection>();
            auto checkAndAddDelta = [this, capturedProfile, capturedGeneration,
                                     knownModIds, parseConn]() -> bool {
                if (capturedGeneration != m_profileSwitchGeneration ||
                    capturedProfile != m_currentProfile) {
                    if (*parseConn)
                        disconnect(*parseConn);
                    return true;
                }
                if (m_model->hasPendingParseTasks())
                    return false;
                if (*parseConn)
                    disconnect(*parseConn);
                QSet<QString> currentModIds;
                for (int i = 0; i < m_model->rowCount(); ++i)
                    currentModIds.insert(m_model->at(i).mod_id());
                QSet<QString> newIds = currentModIds - knownModIds;
                if (!newIds.isEmpty()) {
                    bool profileStillExists = false;
                    for (int i = 0; i < m_profileTabBar->count(); ++i) {
                        if (m_profileTabBar->tabText(i) == capturedProfile) {
                            profileStillExists = true;
                            break;
                        }
                    }
                    if (profileStillExists) {
                        for (const QString& id : std::as_const(newIds))
                            m_profileStates[capturedProfile].insert(id);
                        persistProfileState(capturedProfile);
                        repaintActiveColumn();
                    }
                }
                return true;
            };

            *parseConn = connect(m_model, &ResourceFolderModel::parseFinished, this,
                                 [checkAndAddDelta] { checkAndAddDelta(); });

            connect(m_model, &ResourceFolderModel::updateFinished, this,
                [checkAndAddDelta] { checkAndAddDelta(); },
                Qt::SingleShotConnection);
        }
        m_model->update();
    }
    if (dialog) {
        dialog->deleteLater();
    }
    m_downloadFlowActive = false;
}

void ModFolderPage::updateMods(bool includeDeps)
{
    auto* profile = m_instance->getPackProfile();
    if (!profile->getModLoaders().has_value() && handleNoModLoader()) {
        return;
    }
    if (APPLICATION->settings()->get("ModMetadataDisabled").toBool()) {
        QMessageBox::critical(this, tr("Error"), tr("Mod updates are unavailable when metadata is disabled!"));
        return;
    }
    if (m_instance != nullptr && m_instance->isRunning()) {
        auto response =
            CustomMessageBox::selectable(this, tr("Confirm Update"),
                                         tr("Updating mods while the game is running may cause mod duplication and game crashes.\n"
                                            "The old files may not be deleted as they are in use.\n"
                                            "Are you sure you want to do this?"),
                                         QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
                ->exec();

        if (response != QMessageBox::Yes) {
            return;
        }
    }
    auto selection = m_filterModel->mapSelectionToSource(ui->treeView->selectionModel()->selection()).indexes();

    auto modsList = m_model->selectedResources(selection);
    bool useAll = modsList.empty();
    if (useAll) {
        modsList = m_model->allResources();
    }

    ResourceUpdateDialog updateDialog(this, m_instance, m_model, modsList, includeDeps, profile->getModLoadersList());
    updateDialog.checkCandidates();

    if (updateDialog.aborted()) {
        return;
    }
    if (updateDialog.noUpdates()) {
        QString message{ tr("'%1' is up-to-date! :)").arg(modsList.front()->name()) };
        if (modsList.size() > 1) {
            if (useAll) {
                message = tr("All mods are up-to-date! :)");
            } else {
                message = tr("All selected mods are up-to-date! :)");
            }
        }
        CustomMessageBox::selectable(this, tr("Update checker"), message)->exec();
        return;
    }

    if (updateDialog.exec() != 0) {
        ConcurrentTask tasks("Download Mods", APPLICATION->settings()->get("NumberOfConcurrentDownloads").toInt());
        connect(&tasks, &Task::failed, this, [this](const QString& reason) {
            CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->show();
        });
        connect(&tasks, &Task::succeeded, this, [this, &tasks]() {
            QStringList warnings = tasks.warnings();
            if (warnings.count()) {
                CustomMessageBox::selectable(this, tr("Warnings"), warnings.join('\n'), QMessageBox::Warning)->show();
            }
        });

        for (const auto& task : updateDialog.getTasks()) {
            tasks.addTask(task);
        }

        ProgressDialog loadDialog(this);
        loadDialog.setSkipButton(true, tr("Abort"));
        loadDialog.execWithTask(&tasks);

        m_model->update();
    }
}

void ModFolderPage::deleteModMetadata()
{
    auto selection = m_filterModel->mapSelectionToSource(ui->treeView->selectionModel()->selection()).indexes();
    auto selectionCount = m_model->selectedMods(selection).length();
    if (selectionCount == 0) {
        return;
    }
    if (selectionCount > 1) {
        auto response = CustomMessageBox::selectable(this, tr("Confirm Removal"),
                                                     tr("You are about to remove the metadata for %1 mods.\n"
                                                        "Are you sure?")
                                                         .arg(selectionCount),
                                                     QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
                            ->exec();

        if (response != QMessageBox::Yes) {
            return;
        }
    }

    m_model->deleteMetadata(selection);
}

void ModFolderPage::changeModVersion()
{
    auto* profile = m_instance->getPackProfile();
    if (!profile->getModLoaders().has_value() && handleNoModLoader()) {
        return;
    }
    if (APPLICATION->settings()->get("ModMetadataDisabled").toBool()) {
        QMessageBox::critical(this, tr("Error"), tr("Mod updates are unavailable when metadata is disabled!"));
        return;
    }
    auto selection = m_filterModel->mapSelectionToSource(ui->treeView->selectionModel()->selection()).indexes();
    auto modsList = m_model->selectedMods(selection);
    if (modsList.length() != 1 || modsList[0]->metadata() == nullptr) {
        return;
    }

    m_downloadDialog = ResourceDownload::ResourceDownloadDialog::createMod(this, m_model, m_instance, true);
    connect(this, &QObject::destroyed, m_downloadDialog, &QDialog::close);
    connect(m_downloadDialog, &QDialog::finished, this, &ModFolderPage::downloadDialogFinished);

    m_downloadDialog->setResourceMetadata((*modsList.begin())->metadata());
    m_downloadDialog->open();
}

void ModFolderPage::exportModMetadata()
{
    auto selection = m_filterModel->mapSelectionToSource(ui->treeView->selectionModel()->selection()).indexes();
    auto selectedMods = m_model->selectedMods(selection);
    if (selectedMods.length() == 0) {
        for (auto* mod : m_model->allMods()) {
            if (mod->enabled()) {
                selectedMods.append(mod);
            }
        }
    }

    std::ranges::sort(selectedMods, [](const Mod* a, const Mod* b) { return a->name() < b->name(); });
    ExportToModListDialog dlg(m_instance->name(), selectedMods, this);
    dlg.exec();
}

CoreModFolderPage::CoreModFolderPage(MinecraftInstance* inst, ModFolderModel* mods, QWidget* parent) : ModFolderPage(inst, mods, parent)
{
    auto* version = inst->getPackProfile();
    if ((version != nullptr) && version->getComponent("net.minecraftforge") && version->getComponent("net.minecraft")) {
        auto minecraftCmp = version->getComponent("net.minecraft");
        if (!minecraftCmp->m_loaded) {
            version->reload(Net::Mode::Offline);
            auto update = version->getCurrentTask();
            if (update) {
                connect(update.get(), &Task::finished, this, [this] {
                    if (m_container) {
                        m_container->refreshContainer();
                    }
                });
                if (!update->isRunning()) {
                    update->start();
                }
            }
        }
    }
}

bool CoreModFolderPage::shouldDisplay() const
{
    if (ModFolderPage::shouldDisplay()) {
        auto* version = m_instance->getPackProfile();
        if ((version == nullptr) || !version->getComponent("net.minecraftforge") || !version->getComponent("net.minecraft")) {
            return false;
        }
        auto minecraftCmp = version->getComponent("net.minecraft");
        return minecraftCmp->m_loaded && minecraftCmp->getReleaseDateTime() < g_VersionFilterData.legacyCutoffDate;
    }
    return false;
}

NilModFolderPage::NilModFolderPage(MinecraftInstance* inst, ModFolderModel* mods, QWidget* parent) : ModFolderPage(inst, mods, parent) {}

bool NilModFolderPage::shouldDisplay() const
{
    return m_model->dir().exists();
}

// Helper function so this doesn't need to be duplicated 3 times
inline bool ModFolderPage::handleNoModLoader()
{
    int resp = QMessageBox::question(
        this, ModFolderPage::tr("Missing Mod Loader"),
        ModFolderPage::tr("You need to install a compatible mod loader before installing mods. Would you like to do so?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (resp == QMessageBox::Yes) {
        // Should be safe
        auto* profile = this->m_instance->getPackProfile();
        InstallLoaderDialog dialog(profile, QString(), this);
        // true if the user went through the install loader dialog
        // false if the dialog got canceled/closed
        bool dialogAccepted = dialog.exec() != 0;
        this->m_container->refreshContainer();

        if (!dialogAccepted) {
            return true;
        }
        if (!profile->getModLoaders().has_value()) {
            CustomMessageBox::selectable(this, tr("Error"), tr("No mod loader was installed. Please try again."), QMessageBox::Warning)
                ->show();
            return true;
        }
        return false;
    }
    // Nothing happens the dialog is already closing
    // returning true so the caller doesn't go and continue with opening it's dialog without a mod loader
    return true;
}

bool ModFolderPage::eventFilter(QObject* obj, QEvent* ev)
{
    if (!ui || !ui->treeView || !m_model) {
        return ExternalResourcesPage::eventFilter(obj, ev);
    }
    if (obj == m_profileTabBar && ev->type() == QEvent::Wheel) {
        return true;
    }
    if (ev->type() == QEvent::MouseButtonPress || ev->type() == QEvent::FocusIn) {
        if (this->isVisible()) {
            QWidget* widget = qobject_cast<QWidget*>(obj);
            if (widget) {
                if (!this->isAncestorOf(widget) && widget != this) {
                    ui->treeView->clearSelection();
                    ui->treeView->setCurrentIndex(QModelIndex());
                }
            }
        }
    }
    if (obj == ui->treeView->viewport() && ev->type() == QEvent::MouseButtonPress) {
        auto* me = static_cast<QMouseEvent*>(ev);
        const QModelIndex idx = ui->treeView->indexAt(me->pos());
        if (!idx.isValid()) {
            ui->treeView->clearSelection();
            ui->treeView->setCurrentIndex(QModelIndex());
            return true;
        }
    }
    return ExternalResourcesPage::eventFilter(obj, ev);
}

void ModFolderPage::mousePressEvent(QMouseEvent* event)
{
    if (ui->treeView) {
        QPoint posInTreeView = ui->treeView->mapFrom(this, event->pos());
        if (!ui->treeView->rect().contains(posInTreeView)) {
            ui->treeView->clearSelection();
            ui->treeView->setCurrentIndex(QModelIndex());
        }
    }
    ExternalResourcesPage::mousePressEvent(event);
}

void ModFolderPage::showEvent(QShowEvent* event)
{
    ExternalResourcesPage::showEvent(event);
    QWidget* w = window();
    if (w && m_filterWindow != w) {
        if (m_filterWindow) {
            m_filterWindow->removeEventFilter(this);
        }
        m_filterWindow = w;
        w->installEventFilter(this);
    }
}

void ModFolderPage::hideEvent(QHideEvent* event)
{
    if (m_filterWindow) {
        m_filterWindow->removeEventFilter(this);
        m_filterWindow = nullptr;
    }
    ExternalResourcesPage::hideEvent(event);
}

void ModFolderPage::openedImpl()
{
    ExternalResourcesPage::openedImpl();

    if (ui && ui->actionsToolbar) {
        ui->actionsToolbar->setIconSize(QSize(16, 16));
        for (auto* barAction : ui->actionsToolbar->actions()) {
            barAction->setVisible(true);
            if (QWidget* wid = ui->actionsToolbar->widgetForAction(barAction)) {
                wid->setVisible(true);
                if (auto* btn = qobject_cast<QToolButton*>(wid)) {
                    if (btn->text() == m_actionProfileOverview->text()) {
                        QObject::disconnect(ui->actionsToolbar, &QToolBar::iconSizeChanged,
                                            btn, &QToolButton::setIconSize);
                        btn->setIconSize(QSize(16, 16));
                        btn->setStyleSheet("QToolButton { text-align: left; spacing: 4px; padding-left: 6px; }");
                    }
                }
            }
        }
    }

    const bool isOverview = m_actionProfileOverview && m_actionProfileOverview->isChecked();
    if (ui && ui->actionsToolbar)
        ui->actionsToolbar->setVisible(!isOverview);
}

void ModFolderPage::closedImpl()
{
    if (ui && ui->actionsToolbar) {
        for (auto* barAction : ui->actionsToolbar->actions()) {
            barAction->setVisible(true);
            if (QWidget* wid = ui->actionsToolbar->widgetForAction(barAction))
                wid->setVisible(true);
        }
    }

    ExternalResourcesPage::closedImpl();
}
