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
#include <QMenu>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <algorithm>
#include <memory>

#include "Application.h"

#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ResourceDownloadDialog.h"
#include "ui/dialogs/ResourceUpdateDialog.h"
#include "ui/dialogs/ReviewMessageBox.h"

#include "minecraft/PackProfile.h"
#include "minecraft/VersionFilterData.h"
#include "minecraft/mod/BisectController.h"
#include "minecraft/mod/Mod.h"
#include "minecraft/mod/ModFolderModel.h"
#include "minecraft/mod/ResourceFolderModel.h"

#include "Application.h"
#include "tasks/ConcurrentTask.h"
#include "tasks/Task.h"
#include "ui/dialogs/ProgressDialog.h"

ModFolderPage::ModFolderPage(BaseInstance* inst, ModFolderModel* model, QWidget* parent)
    : ExternalResourcesPage(inst, model, parent), m_model(model)
{
    ui->actionDownloadItem->setText(tr("Download Mods"));
    ui->actionDownloadItem->setToolTip(tr("Download mods from online mod platforms"));
    ui->actionDownloadItem->setEnabled(true);
    ui->actionsToolbar->insertActionBefore(ui->actionAddItem, ui->actionDownloadItem);

    connect(ui->actionDownloadItem, &QAction::triggered, this, &ModFolderPage::downloadMods);

    ui->actionUpdateItem->setToolTip(tr("Try to check or update all selected mods (all mods if none are selected)"));
    connect(ui->actionUpdateItem, &QAction::triggered, this, &ModFolderPage::updateMods);
    ui->actionsToolbar->insertActionBefore(ui->actionAddItem, ui->actionUpdateItem);
    auto* bisectAction = new QAction(tr("Bisect Mods"), this);
    bisectAction->setToolTip(tr("Repeatedly launch the game, disabling/enabling mods, to locate ones causing an issue"));
    ui->actionsToolbar->insertActionBefore(ui->actionAddItem, bisectAction);
    connect(bisectAction, &QAction::triggered, this, &ModFolderPage::bisectMods);

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
        auto response = CustomMessageBox::selectable(this, tr("Confirm Disable"),
                                                     tr("The mods you are trying to delete are required by %1 mods.\n"
                                                        "Do you want to disable them?")
                                                         .arg(affected.length()),
                                                     QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                                                     QMessageBox::Cancel)
                            ->exec();

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

    m_downloadDialog = new ResourceDownload::ModDownloadDialog(this, m_model, m_instance);
    connect(this, &QObject::destroyed, m_downloadDialog, &QDialog::close);
    connect(m_downloadDialog, &QDialog::finished, this, &ModFolderPage::downloadDialogFinished);

    m_downloadDialog->open();
}

void ModFolderPage::downloadDialogFinished(int result)
{
    if (result != 0) {
        auto* tasks = new ConcurrentTask(tr("Download Mods"), APPLICATION->settings()->get("NumberOfConcurrentDownloads").toInt());
        connect(tasks, &Task::failed, [this, tasks](const QString& reason) {
            CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->show();
            tasks->deleteLater();
        });
        connect(tasks, &Task::aborted, [this, tasks]() {
            CustomMessageBox::selectable(this, tr("Aborted"), tr("Download stopped by user."), QMessageBox::Information)->show();
            tasks->deleteLater();
        });
        connect(tasks, &Task::succeeded, [this, tasks]() {
            QStringList warnings = tasks->warnings();
            if (warnings.count()) {
                CustomMessageBox::selectable(this, tr("Warnings"), warnings.join('\n'), QMessageBox::Warning)->show();
            }

            tasks->deleteLater();
        });

        if (m_downloadDialog) {
            for (auto& task : m_downloadDialog->getTasks()) {
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
    if (m_downloadDialog) {
        m_downloadDialog->deleteLater();
    }
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
        connect(tasks, &Task::failed, [this, tasks](const QString& reason) {
            CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->show();
            tasks->deleteLater();
        });
        connect(tasks, &Task::aborted, [this, tasks]() {
            CustomMessageBox::selectable(this, tr("Aborted"), tr("Download stopped by user."), QMessageBox::Information)->show();
            tasks->deleteLater();
        });
        connect(tasks, &Task::succeeded, [this, tasks]() {
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

void ModFolderPage::bisectMods()
{
    QList<Mod*> allMods;
    for (int row = 0; row < m_model->rowCount({}); ++row) {
        allMods << static_cast<Mod*>(&m_model->at(row));
    }

    QSet<Mod*> originallyEnabled;
    for (auto* mod : allMods) {
        if (mod->enabled()) {
            originallyEnabled << mod;
        }
    }

    QList<Mod*> locked = pickLockedMods(allMods);

    QList<Mod*> candidates;
    for (auto* mod : allMods) {
        if (!locked.contains(mod)) {
            candidates << mod;
        }
    }

    auto* bisect = new BisectController(m_instance, m_model, locked, candidates, this);

    auto restoreOriginalState = [this, allMods, originallyEnabled] {
        QSet<Mod*> toEnable, toDisable;
        for (auto* mod : allMods) {
            (originallyEnabled.contains(mod) ? toEnable : toDisable) << mod;
        }
        m_model->setResourceEnabledSilent(toDisable, EnableAction::DISABLE);
        m_model->setResourceEnabledSilent(toEnable, EnableAction::ENABLE);
    };

    auto enableOnlyCulprits = [this, allMods](const QList<Mod*>& culprits) {
        QSet<Mod*> culpritSet(culprits.begin(), culprits.end());
        QSet<Mod*> toEnable, toDisable;
        for (auto* mod : allMods) {
            (culpritSet.contains(mod) || mod->enabled() ? toEnable : toDisable) << mod;
        }
        m_model->setResourceEnabledSilent(toDisable, EnableAction::DISABLE);
        m_model->setResourceEnabledSilent(toEnable, EnableAction::ENABLE);
    };

    connect(bisect, &BisectController::readyToLaunch, this, [this] { APPLICATION->launch(m_instance); }, Qt::QueuedConnection);

    connect(APPLICATION, &Application::instanceLaunchFinished, bisect, &BisectController::onLaunchEnded, Qt::QueuedConnection);

    connect(bisect, &BisectController::promptUser, this, [this, bisect, restoreOriginalState] {
        auto* box = CustomMessageBox::selectable(this, tr("Bisect: does the issue occur?"), tr("Does this mod set reproduce the issue?"),
                                                 QMessageBox::Question,
                                                 QMessageBox::Yes | QMessageBox::No | QMessageBox::Retry | QMessageBox::Abort);
        box->button(QMessageBox::Retry)->setText(tr("Relaunch"));
        box->button(QMessageBox::Abort)->setText(tr("End Bisect"));

        auto response = box->exec();

        if (response == QMessageBox::Abort) {
            bisect->cancel();
            restoreOriginalState();
            bisect->deleteLater();
            return;
        }

        BisectController::Answer answer;
        if (response == QMessageBox::Yes) {
            answer = BisectController::Answer::Yes;
        } else if (response == QMessageBox::Retry) {
            answer = BisectController::Answer::Relaunch;
        } else {
            answer = BisectController::Answer::No;
        }
        bisect->onUserAnswered(answer);
    });

    connect(bisect, &BisectController::finished, this, [this, enableOnlyCulprits](QList<Mod*> culprits) {
        enableOnlyCulprits(culprits);
        QStringList names;
        for (auto* m : culprits)
            names << m->name();
        CustomMessageBox::selectable(this, tr("Bisect complete"), tr("Likely culprit mod(s): %1").arg(names.join(", ")),
                                     QMessageBox::Information)
            ->exec();
    });

    connect(bisect, &BisectController::bailedOut, this, [this, restoreOriginalState](QString reason) {
        restoreOriginalState();
        CustomMessageBox::selectable(this, tr("Bisect stopped"), reason, QMessageBox::Warning)->exec();
    });

    connect(bisect, &BisectController::heisenbugDetected, this, [this, restoreOriginalState](QString set) {
        restoreOriginalState();
        CustomMessageBox::selectable(this, tr("Inconsistent result"),
                                     tr("The same mod set produced different results on retest:\n%1").arg(set), QMessageBox::Warning)
            ->exec();
    });

    bisect->start();
}

QList<Mod*> ModFolderPage::pickLockedMods(const QList<Mod*>& allMods)
{
    auto* dialog = ReviewMessageBox::create(this, tr("Choose mods to lock"));
    dialog->setLabels(tr("Locked mods stay enabled for the whole bisect and are assumed not to be the cause.\n"
                         "Check any mod you want to lock (e.g. one whose feature you're actively testing):"),
                      tr("Unchecked mods will be treated as candidates and may be disabled during the bisect."));

    auto sorted = allMods;
    std::ranges::sort(sorted, [](Mod* a, Mod* b) { return QString::compare(a->name(), b->name(), Qt::CaseInsensitive) < 0; });

    for (auto* mod : sorted) {
        dialog->appendResource({ .name = mod->name(), .filename = mod->fileinfo().fileName(), .enabled = false });
    }

    QList<Mod*> locked;
    if (dialog->exec() != 0) {
        auto deselected = dialog->deselectedResources();
        for (auto* mod : sorted) {
            if (!deselected.contains(mod->name())) {
                locked << mod;
            }
        }
    }
    return locked;
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

    m_downloadDialog = new ResourceDownload::ModDownloadDialog(this, m_model, m_instance, true);
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
        selectedMods = m_model->allMods();
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
