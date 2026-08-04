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
    m_destructorStarted = true;

    if (m_filterWindow) {
        m_filterWindow->removeEventFilter(this);
    }
    if (ui && ui->treeView && ui->treeView->viewport()) {
        ui->treeView->viewport()->removeEventFilter(this);
    }
}

ModFolderPage::ModFolderPage(BaseInstance* inst, ModFolderModel* model, QWidget* parent)
    : ExternalResourcesPage(inst, model, parent), m_model(model)
{
    m_profileTabBar = new QTabBar(this);
    m_profileTabBar->setExpanding(false);
    m_profileTabBar->setDrawBase(false);         // no underline bar — matches Settings tab style
    m_profileTabBar->setDocumentMode(false);
    m_profileTabBar->setUsesScrollButtons(m_profileTabBar->count() > 1);
    m_profileTabBar->setElideMode(Qt::ElideNone);
    m_profileTabBar->setContextMenuPolicy(Qt::CustomContextMenu);
    // Palette-based stylesheet: selected tab uses palette(base) to match the
    // treeView background; unselected tabs use the standard button tone.
    m_profileTabBar->setStyleSheet(
        "QTabBar::tab {"
        "  padding: 4px 12px;"
        "  border: none;"
        "  background: palette(button);"
        "  color: palette(button-text);"
        "  margin-right: 1px;"
        "}"
        "QTabBar::tab:selected {"
        "  background: palette(base);"
        "  color: palette(text);"
        "}"
        "QTabBar::tab:hover:!selected {"
        "  background: palette(mid);"
        "  color: palette(button-text);"
        "}");

    // Chrome-style "+" button that sits immediately after the last tab
    m_newTabButton = new QToolButton(this);
    m_newTabButton->setText(QStringLiteral("+"));
    m_newTabButton->setToolTip(tr("New Tab"));
    m_newTabButton->setAutoRaise(true);
    m_newTabButton->setFixedSize(24, 24);

    if (ui->treeView->parentWidget() && ui->treeView->parentWidget()->layout()) {
        QLayout* layout = ui->treeView->parentWidget()->layout();
        if (auto grid = qobject_cast<QGridLayout*>(layout)) {
            // Container keeps the tab bar + "+" button left-aligned while
            // remaining flush with treeView (row=1, col=1, colSpan=2).
            QWidget* tabRowContainer = new QWidget(this);
            auto* tabRowLayout = new QHBoxLayout(tabRowContainer);
            tabRowLayout->setContentsMargins(0, 0, 0, 0);
            tabRowLayout->setSpacing(0);
            tabRowLayout->addWidget(m_profileTabBar, 0);  // natural tab-bar width
            tabRowLayout->addWidget(m_newTabButton,  0);  // fixed right of last tab
            tabRowLayout->addStretch(1);                  // remaining space left empty

            // The "Active Profiles" button sits at the far right of the tab row.
            // Its popup menu lets users pick which profiles are combined at launch.
            m_runtimeMenu = new QMenu(this);
            m_runtimeProfilesButton = new QToolButton(this);
            m_runtimeProfilesButton->setText(tr("Active Profiles") + QStringLiteral(" ▾"));
            m_runtimeProfilesButton->setToolTip(
                tr("Choose which profiles are combined when launching the game.\n"
                   "If empty, the game uses whatever the editing profile last set."));
            m_runtimeProfilesButton->setPopupMode(QToolButton::InstantPopup);
            m_runtimeProfilesButton->setMenu(m_runtimeMenu);
            // Explicit QSS instead of relying on native autoRaise chrome -- the
            // default rendering had a cramped hitbox and no visible border or
            // hover/press feedback.
            m_runtimeProfilesButton->setAutoRaise(false);
            m_runtimeProfilesButton->setStyleSheet(
                "QToolButton {"
                "  padding: 3px 8px;"
                "  min-height: 18px;"
                "  border: 1px solid palette(mid);"
                "  border-radius: 3px;"
                "  background: palette(button);"
                "  color: palette(button-text);"
                "}"
                "QToolButton:hover {"
                "  background: palette(highlight);"
                "  color: palette(highlighted-text);"
                "  border: 1px solid palette(highlight);"
                "}"
                "QToolButton:pressed {"
                "  background: palette(highlight);"
                "  color: palette(highlighted-text);"
                "}"
                "QToolButton::menu-indicator { image: none; width: 0px; }");
            tabRowLayout->addWidget(m_runtimeProfilesButton, 0);
            qDebug() << "[ModFolderPage] runtime profiles button created"
                     << "page=" << this << "visible=" << m_runtimeProfilesButton->isVisible()
                     << "prefix=" << m_settingsPrefix;

            grid->addWidget(tabRowContainer, 0, 1, 1, 2);
        }
    }

    // Install viewport filter so clicking blank space in the list clears selection.
    ui->treeView->viewport()->installEventFilter(this);
    m_profileTabBar->installEventFilter(this);

    connect(m_profileTabBar, &QTabBar::currentChanged, this, [this](int index) {
        ++m_profileSwitchGeneration;
        applyProfileSwitch(index, m_profileSwitchGeneration);
    });
    connect(m_profileTabBar, &QTabBar::customContextMenuRequested,
            this, &ModFolderPage::onTabContextMenuRequested);
    connect(m_newTabButton, &QToolButton::clicked, this, [this] {
        onTabNewToRight(m_profileTabBar->count() - 1);
    });

    ui->actionDownloadItem->setText(tr("Download Mods"));
    ui->actionDownloadItem->setToolTip(tr("Download mods from online mod platforms"));
    ui->actionDownloadItem->setEnabled(true);
    ui->actionsToolbar->insertActionBefore(ui->actionAddItem, ui->actionDownloadItem);

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

    // Derive the settings key prefix from the model's directory name so that
    // each page type ("mods", "coremods", "nilmods") stores profiles independently.
    m_settingsPrefix = m_model->dir().dirName();

    m_instance->settings()->getOrRegisterSetting(profileListKey(), QStringList() << "All Mods");
    QStringList profileList = m_instance->settings()->get(profileListKey()).toStringList();
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

    m_instance->settings()->getOrRegisterSetting(lastActiveIndexKey(), 0);
    int savedIndex = m_instance->settings()->get(lastActiveIndexKey()).toInt();
    if (savedIndex < 0 || savedIndex >= m_profileTabBar->count()) {
        savedIndex = 0;
    }
    m_profileTabBar->setCurrentIndex(savedIndex);
    applyProfileSwitch(m_profileTabBar->currentIndex(), m_profileSwitchGeneration);

    m_instanceRunning = m_instance->isRunning();
    connect(m_instance, &BaseInstance::runningStatusChanged, this, [this](bool running) {
        qDebug() << "[ModFolderPage] runningStatusChanged"
                 << "running=" << running
                 << "prefix=" << m_settingsPrefix
                 << "timestamp=" << QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
        m_instanceRunning = running;
        if (!running) {
            restoreCurrentProfileToFilesystem();
        }
    });

    // Register the runtime-profiles key and populate the dropdown from the saved selection.
    m_instance->settings()->getOrRegisterSetting(runtimeProfilesKey(), QStringList());
    rebuildRuntimeMenu();

    connect(m_model, &QAbstractItemModel::dataChanged, this, [this]() {
        if (!m_applyingProfile && !m_instanceRunning) {
            saveCurrentProfileState();
        }
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
    if (m_instance->typeName() != "Minecraft") {
        return;  // this is a null instance or a legacy instance
    }

    auto* profile = static_cast<MinecraftInstance*>(m_instance)->getPackProfile();
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
        auto* tasks = new ConcurrentTask(tr("Download Mods"), APPLICATION->settings()->get("NumberOfConcurrentDownloads").toInt());
        auto terminalHandled = std::make_shared<bool>(false);
        connect(tasks, &Task::failed, [this, tasks, terminalHandled](const QString& reason) {
            if (*terminalHandled) {
                return;
            }
            *terminalHandled = true;
            CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->show();
            tasks->deleteLater();
        });
        connect(tasks, &Task::aborted, [this, tasks, terminalHandled]() {
            if (*terminalHandled) {
                return;
            }
            *terminalHandled = true;
            CustomMessageBox::selectable(this, tr("Aborted"), tr("Download stopped by user."), QMessageBox::Information)->show();
            tasks->deleteLater();
        });
        connect(tasks, &Task::succeeded, [this, tasks, terminalHandled]() {
            if (*terminalHandled) {
                return;
            }
            *terminalHandled = true;
            QStringList warnings = tasks->warnings();
            if (warnings.count()) {
                CustomMessageBox::selectable(this, tr("Warnings"), warnings.join('\n'), QMessageBox::Warning)->show();
            }

            tasks->deleteLater();
        });

        if (dialog) {
            for (auto& task : dialog->getTasks()) {
                tasks->addTask(task);
            }
        } else {
            qWarning() << "ResourceDownloadDialog vanished before we could collect tasks!";
        }

        ProgressDialog loadDialog(this);
        loadDialog.setSkipButton(true, tr("Abort"));
        loadDialog.execWithTask(tasks);

        m_model->update();
    }
    if (dialog) {
        dialog->deleteLater();
    }
    m_downloadFlowActive = false;
}

void ModFolderPage::updateMods(bool includeDeps)
{
    if (m_instance->typeName() != "Minecraft") {
        return;  // this is a null instance or a legacy instance
    }

    auto* profile = static_cast<MinecraftInstance*>(m_instance)->getPackProfile();
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
        CustomMessageBox::selectable(this, tr("Aborted"), tr("The mod updater was aborted!"), QMessageBox::Warning)->show();
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
        auto* tasks = new ConcurrentTask("Download Mods", APPLICATION->settings()->get("NumberOfConcurrentDownloads").toInt());
        auto terminalHandled = std::make_shared<bool>(false);
        connect(tasks, &Task::failed, [this, tasks, terminalHandled](const QString& reason) {
            if (*terminalHandled) {
                return;
            }
            *terminalHandled = true;
            CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->show();
            tasks->deleteLater();
        });
        connect(tasks, &Task::aborted, [this, tasks, terminalHandled]() {
            if (*terminalHandled) {
                return;
            }
            *terminalHandled = true;
            CustomMessageBox::selectable(this, tr("Aborted"), tr("Download stopped by user."), QMessageBox::Information)->show();
            tasks->deleteLater();
        });
        connect(tasks, &Task::succeeded, [this, tasks, terminalHandled]() {
            if (*terminalHandled) {
                return;
            }
            *terminalHandled = true;
            QStringList warnings = tasks->warnings();
            if (warnings.count()) {
                CustomMessageBox::selectable(this, tr("Warnings"), warnings.join('\n'), QMessageBox::Warning)->show();
            }
            tasks->deleteLater();
        });

        for (const auto& task : updateDialog.getTasks()) {
            tasks->addTask(task);
        }

        ProgressDialog loadDialog(this);
        loadDialog.setSkipButton(true, tr("Abort"));
        loadDialog.execWithTask(tasks);

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
    if (m_instance->typeName() != "Minecraft") {
        return;  // this is a null instance or a legacy instance
    }

    auto* profile = static_cast<MinecraftInstance*>(m_instance)->getPackProfile();
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
        // Scope to the mods currently enabled by the active profile tab only.
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


CoreModFolderPage::CoreModFolderPage(BaseInstance* inst, ModFolderModel* mods, QWidget* parent) : ModFolderPage(inst, mods, parent)
{
    auto* mcInst = dynamic_cast<MinecraftInstance*>(m_instance);
    if (mcInst) {
        auto* version = mcInst->getPackProfile();
        if ((version != nullptr) && version->getComponent("net.minecraftforge") && version->getComponent("net.minecraft")) {
            auto minecraftCmp = version->getComponent("net.minecraft");
            if (!minecraftCmp->m_loaded) {
                version->reload(Net::Mode::Offline);

                auto update = version->getCurrentTask();
                if (update && update->isRunning()) {
                    connect(update.get(), &Task::finished, this, [this] {
                        if (m_container) {
                            m_container->refreshContainer();
                        }
                    });
                } else if (m_container) {
                    m_container->refreshContainer();
                }
            }
        }
    }
}

bool CoreModFolderPage::shouldDisplay() const
{
    if (ModFolderPage::shouldDisplay()) {
        auto* inst = dynamic_cast<MinecraftInstance*>(m_instance);
        if (!inst) {
            return true;
        }

        auto* version = inst->getPackProfile();
        if ((version == nullptr) || !version->getComponent("net.minecraftforge") || !version->getComponent("net.minecraft")) {
            return false;
        }
        auto minecraftCmp = version->getComponent("net.minecraft");
        return minecraftCmp->m_loaded && minecraftCmp->getReleaseDateTime() < g_VersionFilterData.legacyCutoffDate;
    }
    return false;
}

NilModFolderPage::NilModFolderPage(BaseInstance* inst, ModFolderModel* mods, QWidget* parent) : ModFolderPage(inst, mods, parent) {}

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
        auto* profile = static_cast<MinecraftInstance*>(this->m_instance)->getPackProfile();
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

void ModFolderPage::saveCurrentProfileState(bool refuseSuspiciousEmptyOverwrite)
{
    if (m_currentProfile.isEmpty()) {
        return;
    }
    bool stillExists = false;
    for (int i = 0; i < m_profileTabBar->count(); ++i) {
        if (m_profileTabBar->tabText(i) == m_currentProfile) {
            stillExists = true;
            break;
        }
    }
    if (!stillExists) {
        return;
    }

    QSet<QString> enabledMods;
    bool anyPendingParse = m_model->hasPendingParseTasks();
    bool sawUnresolvedEnabledMod = false;
    for (int i = 0; i < m_model->rowCount(); ++i) {
        const Mod& res = m_model->at(i);
        // Skip mods still resolving metadata: mod_id() returns a placeholder
        // until LocalModParseTask finishes, which is NOT the id
        // applyEnabledIds()/setResourceEnabled() will match against later.
        // Safe to omit here -- ModFolderModel::onParseSucceeded() unconditionally
        // fires dataChanged() for this row once resolved, which re-triggers this
        // save with the correct, final mod_id().
        if (res.enabled() && res.isResolving()) {
            // At least one currently-enabled mod hasn't finished resolving,
            // so we don't have a trustworthy full picture yet. Bail out of
            // the WHOLE save rather than persisting a partial/empty set --
            // writing an empty list here silently overwrites this profile's
            // real saved data (confirmed: produced literal "@Invalid()" on
            // disk for a profile whose only enabled mod was still resolving
            // at save time). This mod's own onParseSucceeded() unconditionally
            // fires dataChanged() for its row once resolved, re-triggering
            // this save with a complete set.
            sawUnresolvedEnabledMod = true;
            continue;
        }
        if (res.enabled()) {
            enabledMods.insert(res.mod_id());
        }
    }
    if (sawUnresolvedEnabledMod) {
        qDebug() << "[ModFolderPage] saveCurrentProfileState deferred -- an enabled mod is still resolving"
                 << "profile=" << m_currentProfile << "prefix=" << m_settingsPrefix;
        return;
    }
    QString key = profileKey(m_currentProfile);
    if (refuseSuspiciousEmptyOverwrite && enabledMods.isEmpty() && !anyPendingParse) {
        auto stored =
            m_instance->settings()->getOrRegisterSetting(key, QStringList())->get().toStringList();
        if (!stored.isEmpty()) {
            qDebug() << "[ModFolderPage] refusing automatic re-apply save: computed enabled set is empty"
                     << "but stored profile is non-empty"
                     << "profile=" << m_currentProfile
                     << "storedCount=" << stored.size()
                     << "key=" << key
                     << "prefix=" << m_settingsPrefix;
            return;
        }
    }
    m_profileStates[m_currentProfile] = enabledMods;

    m_instance->settings()->getOrRegisterSetting(key, QStringList());
    m_instance->settings()->set(key, QStringList(enabledMods.begin(), enabledMods.end()));
}
// Called when the game process exits (runningStatusChanged(false)). The
// launch path (ScanModFolders::applyRuntimeProfiles -> applyEnabledIds)
// mutates the filesystem to the temporary runtime-profile union (or the
// last-active editing profile) for the duration of the launch. This page
// object persists for as long as it exists, but it does NOT exist at all
// for a launch started from the main window without Edit Instance ever
// being opened -- ScanModFolders now also registers its own UI-independent
// restore trigger for exactly that case (see applyRuntimeProfiles()). This
// function remains as a redundant safety net while a page happens to be
// open; nothing else ever puts the filesystem back, so if this page
// the leftover union as the "real" current state and the normal, correctly
// -guarded dataChanged -> saveCurrentProfileState() path will faithfully
// persist that union into whichever profile happens to be open. This
// restores the filesystem to match the currently-open profile's last saved
// state as soon as the game exits, so any later rescan finds nothing to
// (incorrectly) save.
void ModFolderPage::restoreCurrentProfileToFilesystem()
{
    if (m_currentProfile.isEmpty()) {
        return;
    }

    QSet<QString> savedMods = m_profileStates.value(m_currentProfile);

    QModelIndexList toEnable;
    QModelIndexList toDisable;
    for (int i = 0; i < m_model->rowCount(); ++i) {
        const Mod& res = m_model->at(i);
        QModelIndex idx = m_model->index(i, 0);
        bool shouldBeEnabled = savedMods.contains(res.mod_id());
        if (shouldBeEnabled && !res.enabled()) {
            toEnable.append(idx);
        } else if (!shouldBeEnabled && res.enabled()) {
            toDisable.append(idx);
        }
    }

    if (toEnable.isEmpty() && toDisable.isEmpty()) {
        return;  // filesystem already matches the saved profile -- no runtime override was applied, or it already matched
    }

    qDebug() << "[ModFolderPage] restoring filesystem to saved profile after game exit"
             << "profile=" << m_currentProfile << "toEnable=" << toEnable.size()
             << "toDisable=" << toDisable.size() << "prefix=" << m_settingsPrefix;

    // Suppress the dataChanged-triggered save while we perform this
    // corrective write -- the filesystem is about to match
    // m_profileStates[m_currentProfile] exactly, so there is nothing new to
    // persist once it settles.
    m_applyingProfile = true;
    // Bypass ModFolderModel::setResourceEnabled()'s dependency-cascade dialog --
    // this is an automatic background correction the user never asked for and
    // has no dialog-worthy "action" to confirm. Same reasoning as
    // applyProfileSwitch() and the ScanModFolders launch path.
    m_model->applyEnabledIds(savedMods);

    connect(m_model, &ResourceFolderModel::updateFinished, this,
        [this] { m_applyingProfile = false; },
        Qt::SingleShotConnection);
    m_model->update();
}

void ModFolderPage::applyProfileSwitch(int index, int generation) {
    if (m_currentProfile.isEmpty()) {
        QVariant val = m_profileTabBar->property("currentProfileName");
        m_currentProfile = val.isValid() ? val.toString() : QString();
    }

    if (!m_instanceRunning) {
        saveCurrentProfileState(/*refuseSuspiciousEmptyOverwrite=*/true);
    }

    if (index >= 0 && index < m_profileTabBar->count()) {
        if (m_instance != nullptr && m_instance->isRunning()) {
            CustomMessageBox::selectable(this, tr("Cannot Switch Profile"),
                                         tr("Mod profiles cannot be switched while the game is running.\n"
                                            "Please close the game first."),
                                         QMessageBox::Warning)
                ->exec();
            int previousIndex = m_instance->settings()->get(lastActiveIndexKey()).toInt();
            if (m_profileTabBar->currentIndex() != previousIndex) {
                m_profileTabBar->blockSignals(true);
                m_profileTabBar->setCurrentIndex(previousIndex);
                m_profileTabBar->blockSignals(false);
            }
            return;
        }

        QString tabName = m_profileTabBar->tabText(index);
        m_currentProfile = tabName;
        m_profileTabBar->setProperty("currentProfileName", tabName);
        m_instance->settings()->set(lastActiveIndexKey(), index);

        QSet<QString> enabledMods;
        if (m_profileStates.contains(tabName)) {
            enabledMods = m_profileStates[tabName];
        } else {
            QString key = profileKey(tabName);
            m_instance->settings()->getOrRegisterSetting(key, QStringList());
            QStringList saved = m_instance->settings()->get(key).toStringList();
            enabledMods = QSet<QString>(saved.begin(), saved.end());
            m_profileStates[tabName] = enabledMods;
        }

        m_applyingProfile = true;
        // Apply the target profile's exact enabled-mod set directly, bypassing
        // ModFolderModel::setResourceEnabled()'s dependency-cascade confirmation
        // dialog. That dialog exists for a user manually toggling a mod's
        // checkbox; a profile *switch* is not that -- it deterministically
        // restores a previously-saved set, and a popup firing mid-switch (with
        // no clear "which mod did I toggle" context for the user) is a bug, not
        // a safety feature. applyEnabledIds() already exists for exactly this
        // purpose -- the launch path (ScanModFolders) uses it for the same
        // reason; see its doc comment in ModFolderModel.cpp.
        m_model->applyEnabledIds(enabledMods);

        // Capture generation token. If tab changes again before this update
        // completes, the token will be stale and we discard the result.
        int capturedGeneration = generation;
        connect(m_model, &ResourceFolderModel::updateFinished, this,
            [this, capturedGeneration] {
                if (capturedGeneration == m_profileSwitchGeneration) {
                    saveCurrentProfileState(/*refuseSuspiciousEmptyOverwrite=*/true);
                }
                m_applyingProfile = false;
            },
            Qt::SingleShotConnection);

        m_model->update();

    } else {
        m_currentProfile = QString();
        m_profileTabBar->setProperty("currentProfileName", QString());
        m_applyingProfile = false;
    }
}


void ModFolderPage::onAddProfileClicked() {
    bool ok;
    QString name = QInputDialog::getText(this, tr("Add Profile"), tr("Profile Name:"),
                                         QLineEdit::Normal, QString(), &ok);
    if (ok && !name.isEmpty()) {
        // Always create a blank-slate profile — no state bleeding from the current profile.
        createProfile(name, QSet<QString>{});
    }
}

void ModFolderPage::onRemoveProfileClicked() {
    int index = m_profileTabBar->currentIndex();
    // Guard: index 0 is the base profile and must never be removed.
    if (index > 0) {
        onTabRemove(index);
    }
}

// ---------------------------------------------------------------------------
// Profile creation helpers
// ---------------------------------------------------------------------------

void ModFolderPage::createProfile(const QString& name, const QSet<QString>& initialState,
                                   int insertAfterIndex)
{
    // Guard: duplicate name
    for (int i = 0; i < m_profileTabBar->count(); ++i) {
        if (m_profileTabBar->tabText(i) == name) {
            QMessageBox::warning(this, tr("Warning"),
                                 tr("A profile with this name already exists."));
            return;
        }
    }

    // Persist state to memory and settings
    m_profileStates[name] = initialState;
    QString key = profileKey(name);

    m_instance->settings()->getOrRegisterSetting(key, QStringList());
    m_instance->settings()->set(key, QStringList(initialState.begin(), initialState.end()));

    // Insert tab at the correct position
    if (insertAfterIndex >= 0 && insertAfterIndex < m_profileTabBar->count()) {
        m_profileTabBar->insertTab(insertAfterIndex + 1, name);
    } else {
        m_profileTabBar->addTab(name);
    }
    m_profileTabBar->setUsesScrollButtons(m_profileTabBar->count() > 1);

    saveProfileList();
    rebuildRuntimeMenu();
}

void ModFolderPage::saveProfileList()
{
    QStringList profileList;
    for (int i = 0; i < m_profileTabBar->count(); ++i) {
        profileList.append(m_profileTabBar->tabText(i));
    }
    m_instance->settings()->set(profileListKey(), profileList);
}

// ---------------------------------------------------------------------------
// Tab context-menu
// ---------------------------------------------------------------------------

void ModFolderPage::onTabContextMenuRequested(const QPoint& pos)
{
    int tabIndex = m_profileTabBar->tabAt(pos);

    QMenu menu(this);

    // ── Management group ────────────────────────────────────────────────────
    auto* newToRight = menu.addAction(tr("New Tab to Right"));
    int insertAfter = tabIndex >= 0 ? tabIndex : m_profileTabBar->count() - 1;
    connect(newToRight, &QAction::triggered, this, [this, insertAfter] {
        onTabNewToRight(insertAfter);
    });

    if (tabIndex >= 0) {
        auto* duplicate = menu.addAction(tr("Duplicate"));
        connect(duplicate, &QAction::triggered, this, [this, tabIndex] {
            onTabDuplicate(tabIndex);
        });

        // Rename and Remove are permanently blocked for the base profile (index 0).
        if (tabIndex != 0) {
            auto* rename = menu.addAction(tr("Rename"));
            connect(rename, &QAction::triggered, this, [this, tabIndex] {
                onTabRename(tabIndex);
            });

            auto* remove = menu.addAction(tr("Remove"));
            connect(remove, &QAction::triggered, this, [this, tabIndex] {
                onTabRemove(tabIndex);
            });
        }

        // ── Selection / bulk-action group ────────────────────────────────────
        menu.addSeparator();

        auto* enableAll = menu.addAction(tr("Enable All"));
        connect(enableAll, &QAction::triggered, this, [this, tabIndex] {
            onTabEnableAll(tabIndex);
        });

        auto* disableAll = menu.addAction(tr("Disable All"));
        connect(disableAll, &QAction::triggered, this, [this, tabIndex] {
            onTabDisableAll(tabIndex);
        });
    }

    menu.exec(m_profileTabBar->mapToGlobal(pos));
}

void ModFolderPage::onTabNewToRight(int sourceIndex)
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("New Profile"), tr("Profile Name:"),
                                         QLineEdit::Normal, QString(), &ok);
    if (ok && !name.isEmpty()) {
        createProfile(name, QSet<QString>{}, sourceIndex);
    }
}

void ModFolderPage::onTabDuplicate(int sourceIndex)
{
    // Flush the live state of the currently active profile before reading source.
    if (!m_currentProfile.isEmpty()) {
        QSet<QString> enabledMods;
        for (int i = 0; i < m_model->rowCount(); ++i) {
            const Mod& res = m_model->at(i);
            if (res.enabled()) {
                enabledMods.insert(res.mod_id());
            }
        }
        m_profileStates[m_currentProfile] = enabledMods;
    }

    QString sourceName = m_profileTabBar->tabText(sourceIndex);
    QSet<QString> sourceState = m_profileStates.value(sourceName);

    bool ok;
    QString name = QInputDialog::getText(this, tr("Duplicate Profile"),
                                          tr("New Profile Name:"), QLineEdit::Normal,
                                          tr("Copy of %1").arg(sourceName), &ok);
    if (ok && !name.isEmpty()) {
        createProfile(name, sourceState, sourceIndex);
    }
}

void ModFolderPage::onTabRename(int tabIndex)
{
    if (tabIndex == 0) return;  // Guard: base profile cannot be renamed

    if (m_instance != nullptr && m_instance->isRunning()) {
        CustomMessageBox::selectable(this, tr("Cannot Rename Profile"),
                                     tr("Mod profiles cannot be renamed while the game is running.\n"
                                        "Please close the game first."),
                                     QMessageBox::Warning)
            ->exec();
        return;
    }

    QString oldName = m_profileTabBar->tabText(tabIndex);

    bool ok;
    QString newName = QInputDialog::getText(this, tr("Rename Profile"),
                                             tr("New Profile Name:"), QLineEdit::Normal,
                                             oldName, &ok);

    if (!ok || newName.isEmpty() || newName == oldName) return;

    // Validate: no duplicate names
    for (int i = 0; i < m_profileTabBar->count(); ++i) {
        if (m_profileTabBar->tabText(i) == newName) {
            QMessageBox::warning(this, tr("Warning"),
                                 tr("A profile named \"%1\" already exists.").arg(newName));
            return;
        }
    }

    // Migrate in-memory state to the new key
    if (m_profileStates.contains(oldName)) {
        m_profileStates[newName] = m_profileStates.take(oldName);
    }

    // Migrate settings: read → reset old key → write under new key
    QString oldKey = profileKey(oldName);
    QString newKey = profileKey(newName);
    QStringList stateData = m_instance->settings()->get(oldKey).toStringList();
    m_instance->settings()->reset(oldKey);
    m_instance->settings()->getOrRegisterSetting(newKey, QStringList());
    m_instance->settings()->set(newKey, stateData);

    // Update the visible tab label
    m_profileTabBar->setTabText(tabIndex, newName);

    // Keep current-profile tracking in sync
    if (m_currentProfile == oldName) {
        m_currentProfile = newName;
        m_profileTabBar->setProperty("currentProfileName", newName);
    }

    saveProfileList();
    rebuildRuntimeMenu();
}

void ModFolderPage::onTabRemove(int tabIndex)
{
    if (tabIndex == 0) return;  // Guard: base profile cannot be removed

    if (m_instance != nullptr && m_instance->isRunning()) {
        CustomMessageBox::selectable(this, tr("Cannot Remove Profile"),
                                     tr("Mod profiles cannot be removed while the game is running.\n"
                                        "Please close the game first."),
                                     QMessageBox::Warning)
            ->exec();
        return;
    }

    QString name = m_profileTabBar->tabText(tabIndex);

    auto response = CustomMessageBox::selectable(
                        this, tr("Remove Profile"),
                        tr("Are you sure you want to remove the profile \"%1\"?\n"
                           "This cannot be undone.").arg(name),
                        QMessageBox::Warning,
                        QMessageBox::Yes | QMessageBox::No, QMessageBox::No)->exec();

    if (response != QMessageBox::Yes) return;

    m_profileStates.remove(name);

    m_instance->settings()->reset(profileKey(name));

    // Also drop this profile from the runtime (Active Profiles) selection if
    // present -- otherwise a deleted profile's name lingers as a dangling
    // reference in ModRuntimeProfiles_<prefix> even though its own key was
    // just reset().
    QStringList runtimeSelected = loadRuntimeSelection();
    if (runtimeSelected.removeAll(name) > 0) {
        saveRuntimeSelection(runtimeSelected);
    }

    m_profileTabBar->removeTab(tabIndex);
    m_profileTabBar->setUsesScrollButtons(m_profileTabBar->count() > 1);
    saveProfileList();
    rebuildRuntimeMenu();
}

void ModFolderPage::onTabEnableAll(int tabIndex)
{
    if (m_applyingProfile) {
        return;
    }
    if (m_instance != nullptr && m_instance->isRunning()) {
        CustomMessageBox::selectable(this, tr("Cannot Enable All"),
                                     tr("Mods cannot be bulk-enabled while the game is running.\n"
                                        "Please close the game first."),
                                     QMessageBox::Warning)
            ->exec();
        return;
    }

    ++m_profileSwitchGeneration;
    int capturedGeneration = m_profileSwitchGeneration;

    // Block signals to prevent currentChanged from double-firing applyProfileSwitch
    if (m_profileTabBar->currentIndex() != tabIndex) {
        m_profileTabBar->blockSignals(true);
        m_profileTabBar->setCurrentIndex(tabIndex);
        m_profileTabBar->blockSignals(false);
    }

    applyProfileSwitch(tabIndex, capturedGeneration);

    // Chain enable-all after profile switch update finishes
    connect(m_model, &ResourceFolderModel::updateFinished, this,
        [this, capturedGeneration] {
            if (capturedGeneration != m_profileSwitchGeneration) {
                return;
            }
            QModelIndexList allIndices;
            allIndices.reserve(m_model->rowCount());
            for (int i = 0; i < m_model->rowCount(); ++i) {
                allIndices.append(m_model->index(i, 0));
            }
            if (!allIndices.isEmpty()) {
                m_model->setResourceEnabled(allIndices, EnableAction::ENABLE);
                m_model->update();
            }
        },
        Qt::SingleShotConnection);
}

void ModFolderPage::onTabDisableAll(int tabIndex)
{
    if (m_applyingProfile) {
        return;
    }
    if (m_instance != nullptr && m_instance->isRunning()) {
        CustomMessageBox::selectable(this, tr("Cannot Disable All"),
                                     tr("Mods cannot be bulk-disabled while the game is running.\n"
                                        "Please close the game first."),
                                     QMessageBox::Warning)
            ->exec();
        return;
    }

    ++m_profileSwitchGeneration;
    int capturedGeneration = m_profileSwitchGeneration;

    // Block signals to prevent currentChanged from double-firing applyProfileSwitch
    if (m_profileTabBar->currentIndex() != tabIndex) {
        m_profileTabBar->blockSignals(true);
        m_profileTabBar->setCurrentIndex(tabIndex);
        m_profileTabBar->blockSignals(false);
    }

    applyProfileSwitch(tabIndex, capturedGeneration);

    // Chain disable-all after profile switch update finishes
    connect(m_model, &ResourceFolderModel::updateFinished, this,
        [this, capturedGeneration] {
            if (capturedGeneration != m_profileSwitchGeneration) {
                return;
            }
            QModelIndexList allIndices;
            allIndices.reserve(m_model->rowCount());
            for (int i = 0; i < m_model->rowCount(); ++i) {
                allIndices.append(m_model->index(i, 0));
            }
            if (!allIndices.isEmpty()) {
                m_model->setResourceEnabled(allIndices, EnableAction::DISABLE);
                m_model->update();
            }
        },
        Qt::SingleShotConnection);
}

bool ModFolderPage::eventFilter(QObject* obj, QEvent* ev)
{
    // Safety guard: ensure the page, its UI, and tree view are fully valid
    if (!ui || !ui->treeView || !m_model) {
        return ExternalResourcesPage::eventFilter(obj, ev);
    }

    if (obj == m_profileTabBar && ev->type() == QEvent::Wheel) {
        return true;  // swallow wheel events so they can't change the active profile tab
    }

    // When focus or clicks land outside this page (e.g. the instance sidebar), clear selection.
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

    // Clear treeView selection when the user clicks on empty space below the
    // last row, so that Export List reverts to the full active-profile scope.
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

// ---------------------------------------------------------------------------
// Runtime profile selection
// ---------------------------------------------------------------------------

void ModFolderPage::rebuildRuntimeMenu()
{
    if (!m_runtimeMenu)
        return;

    QStringList selected = loadRuntimeSelection();
    m_runtimeMenu->clear();

    for (int i = 0; i < m_profileTabBar->count(); ++i) {
        QString name = m_profileTabBar->tabText(i);
        QAction* action = m_runtimeMenu->addAction(name);
        action->setCheckable(true);
        action->setChecked(selected.contains(name));
        connect(action, &QAction::triggered, this, [this, name](bool checked) {
            onRuntimeProfileToggled(name, checked);
        });
    }
}

void ModFolderPage::onRuntimeProfileToggled(const QString& name, bool checked)
{
    QStringList selected = loadRuntimeSelection();

    if (checked) {
        // Tab 0 ("All Mods" or whatever the base profile is named) is mutually
        // exclusive with all other runtime selections to keep the launch intent clear.
        bool isBaseProfile = (m_profileTabBar->count() > 0 && name == m_profileTabBar->tabText(0));
        if (isBaseProfile) {
            selected.clear();
        } else {
            // Selecting any non-base profile deselects the base profile.
            if (m_profileTabBar->count() > 0)
                selected.removeAll(m_profileTabBar->tabText(0));
        }
        if (!selected.contains(name))
            selected.append(name);
    } else {
        selected.removeAll(name);
    }

    saveRuntimeSelection(selected);

    // Sync all checkmarks in the existing menu without rebuilding it.
    for (QAction* action : m_runtimeMenu->actions())
        action->setChecked(selected.contains(action->text()));
}

QStringList ModFolderPage::loadRuntimeSelection() const
{
    return m_instance->settings()->get(runtimeProfilesKey()).toStringList();
}

void ModFolderPage::saveRuntimeSelection(const QStringList& selected)
{
    m_instance->settings()->set(runtimeProfilesKey(), selected);
}
