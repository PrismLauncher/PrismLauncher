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
#include "ui/dialogs/ExportToModListDialog.h"
#include "ui/dialogs/InstallLoaderDialog.h"
#include "ui_ExternalResourcesPage.h"

#include <QAbstractItemModel>
#include <QAction>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QHash>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QSet>
#include <QSortFilterProxyModel>
#include <algorithm>
#include <memory>
#include <vector>

#include "Application.h"
#include "DesktopServices.h"
#include "FileSystem.h"

#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ResourceDownloadDialog.h"
#include "ui/dialogs/ResourceUpdateDialog.h"

#include "minecraft/PackProfile.h"
#include "minecraft/VersionFilterData.h"
#include "minecraft/mod/Mod.h"
#include "minecraft/mod/ModFolderModel.h"
#include "minecraft/mod/ModFolderTreeModel.h"

#include "tasks/ConcurrentTask.h"
#include "tasks/Task.h"
#include "ui/dialogs/ProgressDialog.h"

ModFolderPage::ModFolderPage(BaseInstance* inst, std::shared_ptr<ModFolderModel> model, QWidget* parent)
    : ExternalResourcesPage(inst, model, parent), m_model(model)
{
    ui->actionDownloadItem->setText(tr("Download Mods"));
    ui->actionDownloadItem->setToolTip(tr("Download mods from online mod platforms"));
    ui->actionDownloadItem->setEnabled(true);
    ui->actionsToolbar->insertActionBefore(ui->actionAddItem, ui->actionDownloadItem);

    connect(ui->actionDownloadItem, &QAction::triggered, this, &ModFolderPage::downloadMods);

    ui->actionUpdateItem->setToolTip(tr("Try to check or update all selected mods (root mods if none are selected)"));
    connect(ui->actionUpdateItem, &QAction::triggered, this, &ModFolderPage::updateMods);
    ui->actionsToolbar->insertActionBefore(ui->actionAddItem, ui->actionUpdateItem);

    auto updateMenu = new QMenu(this);

    auto update = updateMenu->addAction(tr("Check for Updates"));
    connect(update, &QAction::triggered, this, &ModFolderPage::updateMods);

    updateMenu->addAction(ui->actionVerifyItemDependencies);
    connect(ui->actionVerifyItemDependencies, &QAction::triggered, this, [this] { updateMods(true); });

    auto depsDisabled = APPLICATION->settings()->getSetting("ModDependenciesDisabled");
    ui->actionVerifyItemDependencies->setVisible(!depsDisabled->get().toBool());
    connect(depsDisabled.get(), &Setting::SettingChanged, this, [this](const Setting& setting, const QVariant& value) {
        Q_UNUSED(setting);
        ui->actionVerifyItemDependencies->setVisible(!value.toBool());
    });

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

    m_newFolderAction = new QAction(tr("New Folder"), this);
    connect(m_newFolderAction, &QAction::triggered, this, &ModFolderPage::createFolder);
    ui->actionsToolbar->insertActionAfter(ui->actionAddItem, m_newFolderAction);

    m_treeModel = new ModFolderTreeModel(m_model.get(), this);
    m_treeFilterModel = new ModFolderTreeProxyModel(this);
    m_treeFilterModel->setDynamicSortFilter(true);
    m_treeFilterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_treeFilterModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_treeFilterModel->setSourceModel(m_treeModel);
    m_treeFilterModel->setFilterKeyColumn(-1);

    m_filterModel = m_treeFilterModel;
    ui->treeView->setModel(m_filterModel);
    ui->treeView->setResizeModes(m_model->columnResizeModes());
    ui->treeView->setRootIsDecorated(true);
    ui->treeView->setItemsExpandable(true);
    ui->treeView->setExpandsOnDoubleClick(true);
    ui->treeView->setSortingEnabled(false);
    ui->treeView->setAnimated(false);

    disconnect(ui->actionRemoveItem, &QAction::triggered, this, nullptr);
    connect(ui->actionRemoveItem, &QAction::triggered, this, &ModFolderPage::removeItem);

    disconnect(ui->treeView, &ModListView::activated, this, nullptr);
    connect(ui->treeView, &ModListView::activated, this, &ModFolderPage::itemActivated);

    auto selectionModel = ui->treeView->selectionModel();
    connect(selectionModel, &QItemSelectionModel::currentChanged, this, &ModFolderPage::updateFrame);

    auto updateExtra = [this]() {
        if (updateExtraInfo) {
            updateExtraInfo(id(), extraHeaderInfoString());
        }
    };
    connect(selectionModel, &QItemSelectionModel::selectionChanged, this, updateExtra);
    connect(selectionModel, &QItemSelectionModel::selectionChanged, this, &ModFolderPage::updateActions);

    updateActions();
}

bool ModFolderPage::shouldDisplay() const
{
    return true;
}

QModelIndexList ModFolderPage::selectedSourceIndexes() const
{
    return m_filterModel->mapSelectionToSource(ui->treeView->selectionModel()->selection()).indexes();
}

QList<Resource*> ModFolderPage::selectedResources() const
{
    return m_treeModel->resourcesFromIndexes(selectedSourceIndexes());
}

QList<Mod*> ModFolderPage::selectedMods() const
{
    return m_treeModel->modsFromIndexes(selectedSourceIndexes());
}

QList<Resource*> ModFolderPage::rootResourcesForUpdate() const
{
    QList<Resource*> resources;
    QSet<QString> seen;
    const auto rootPath = m_model->dir().absolutePath();
    for (auto* resource : m_model->allResources()) {
        if (resource->fileinfo().absolutePath() != rootPath) {
            continue;
        }
        if (seen.contains(resource->internal_id())) {
            continue;
        }
        seen.insert(resource->internal_id());
        resources.append(resource);
    }
    return resources;
}

QList<Resource*> ModFolderPage::resourcesForUpdateSelection(const QModelIndexList& sourceIndexes) const
{
    if (sourceIndexes.isEmpty()) {
        return rootResourcesForUpdate();
    }

    QList<Resource*> resources;
    QSet<QString> seen;
    auto addResource = [&resources, &seen](Resource* resource) {
        if (!resource) {
            return;
        }
        if (seen.contains(resource->internal_id())) {
            return;
        }
        seen.insert(resource->internal_id());
        resources.append(resource);
    };

    for (auto* resource : m_treeModel->resourcesFromIndexes(sourceIndexes)) {
        addResource(resource);
    }

    for (const auto& index : sourceIndexes) {
        if (index.column() != 0) {
            continue;
        }
        if (!m_treeModel->isFolderIndex(index)) {
            continue;
        }
        const int rowCount = m_treeModel->rowCount(index);
        for (int row = 0; row < rowCount; ++row) {
            auto childIndex = m_treeModel->index(row, 0, index);
            addResource(m_treeModel->resourceForIndex(childIndex));
        }
    }

    return resources;
}

QStringList ModFolderPage::topLevelFolderPaths(const QModelIndexList& sourceIndexes) const
{
    QSet<QString> folderPaths;
    for (const auto& index : sourceIndexes) {
        if (index.column() != 0) {
            continue;
        }
        if (!m_treeModel->isFolderIndex(index)) {
            continue;
        }
        auto dir = m_treeModel->folderForIndex(index);
        if (dir.dirName() == ".index") {
            continue;
        }
        auto path = dir.absolutePath();
        if (path == m_model->dir().absolutePath()) {
            continue;
        }
        folderPaths.insert(path);
    }

    QStringList prunedFolders = folderPaths.values();
    std::sort(prunedFolders.begin(), prunedFolders.end(), [](const QString& left, const QString& right) {
        if (left.size() != right.size()) {
            return left.size() < right.size();
        }
        return left < right;
    });

    QStringList foldersToRemove;
    for (const auto& path : prunedFolders) {
        bool isChild = false;
        for (const auto& parent : foldersToRemove) {
            if (path == parent || path.startsWith(parent + "/")) {
                isChild = true;
                break;
            }
        }
        if (!isChild) {
            foldersToRemove.append(path);
        }
    }

    return foldersToRemove;
}

void ModFolderPage::showFolderRemovalErrors(const QStringList& failedFolders)
{
    if (failedFolders.isEmpty()) {
        return;
    }

    QStringList entries;
    entries.reserve(failedFolders.size());
    for (const auto& folder : failedFolders) {
        entries.append(QFileInfo(folder).fileName());
    }

    CustomMessageBox::selectable(this, tr("Error"),
                                 tr("Failed to remove the following folder(s):\n%1").arg(entries.join('\n')),
                                 QMessageBox::Warning)
        ->show();
}

void ModFolderPage::updateActions()
{
    const auto selection = selectedSourceIndexes();
    const auto selectedResources = m_treeModel->resourcesFromIndexes(selection);

    const bool hasModSelection = !selectedResources.isEmpty();
    bool hasFolderSelection = false;
    for (const auto& index : selection) {
        if (index.column() != 0) {
            continue;
        }
        if (m_treeModel->isFolderIndex(index)) {
            hasFolderSelection = true;
            break;
        }
    }

    ui->actionUpdateItem->setEnabled(!m_model->empty());
    ui->actionResetItemMetadata->setEnabled(hasModSelection);

    ui->actionChangeVersion->setEnabled(selectedResources.size() == 1 && selectedResources[0]->metadata() != nullptr);

    ui->actionRemoveItem->setEnabled(hasModSelection || hasFolderSelection);
    ui->actionEnableItem->setEnabled(hasModSelection);
    ui->actionDisableItem->setEnabled(hasModSelection);

    ui->actionViewHomepage->setEnabled(hasModSelection &&
                                       std::any_of(selectedResources.begin(), selectedResources.end(),
                                                   [](Resource* resource) { return resource && !resource->homepage().isEmpty(); }));
    ui->actionExportMetadata->setEnabled(!m_model->empty());

    if (m_treeModel->hasFolderNodes()) {
        ui->actionUpdateItem->setToolTip(tr("Try to check or update all selected mods (root mods only unless folders/mods are selected)"));
    } else {
        ui->actionUpdateItem->setToolTip(tr("Try to check or update all selected mods (all mods if none are selected)"));
    }

    if (m_newFolderAction) {
        m_newFolderAction->setEnabled(m_instance && m_instance->typeName() == "Minecraft");
    }
}

void ModFolderPage::updateFrame(const QModelIndex& current, [[maybe_unused]] const QModelIndex& previous)
{
    auto sourceCurrent = m_filterModel->mapToSource(current);
    if (!sourceCurrent.isValid()) {
        ui->frame->clear();
        return;
    }
    auto* mod = m_treeModel->modForIndex(sourceCurrent);
    if (!mod) {
        ui->frame->clear();
        return;
    }
    ui->frame->updateWithMod(*mod);
}

void ModFolderPage::removeItem()
{
    auto selection = m_filterModel->mapSelectionToSource(ui->treeView->selectionModel()->selection());
    auto selectionIndexes = selection.indexes();
    auto resources = m_treeModel->resourcesFromIndexes(selectionIndexes);
    auto foldersToRemove = topLevelFolderPaths(selectionIndexes);

    if (resources.isEmpty() && foldersToRemove.isEmpty()) {
        return;
    }

    if (!foldersToRemove.isEmpty() || !resources.isEmpty()) {
        QString message;
        if (foldersToRemove.size() == 1 && resources.isEmpty()) {
            message = tr("You are about to remove the folder '%1'.\n"
                         "This will delete the folder and all its contents.\n\n"
                         "Are you sure?")
                          .arg(QFileInfo(foldersToRemove.front()).fileName());
        } else {
            QStringList parts;
            if (!foldersToRemove.isEmpty()) {
                parts << tr("%1 folder(s)").arg(foldersToRemove.size());
            }
            if (!resources.isEmpty()) {
                parts << tr("%1 item(s)").arg(resources.size());
            }
            message = tr("You are about to remove %1.\n"
                         "This may be permanent and the files will be gone from the folder.\n\n"
                         "Are you sure?")
                          .arg(parts.join(tr(" and ")));
        }

        auto response = CustomMessageBox::selectable(this, tr("Confirm Removal"), message, QMessageBox::Warning,
                                                     QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
                            ->exec();
        if (response != QMessageBox::Yes) {
            return;
        }
    }

    removeItems(selection);
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
    auto selectionIndexes = selection.indexes();
    auto resources = m_treeModel->resourcesFromIndexes(selectionIndexes);
    auto indexes = m_model->indexesForResources(resources);
    if (!indexes.isEmpty()) {
        m_model->deleteResources(indexes);
    }

    auto foldersToRemove = topLevelFolderPaths(selectionIndexes);
    QStringList failedFolders;
    for (const auto& folder : foldersToRemove) {
        if (!FS::deletePath(folder)) {
            failedFolders.append(folder);
        }
    }

    showFolderRemovalErrors(failedFolders);
    m_model->update();
}

void ModFolderPage::enableItem()
{
    auto resources = selectedResources();
    auto indexes = m_model->indexesForResources(resources);
    m_model->setResourceEnabled(indexes, EnableAction::ENABLE);
}

void ModFolderPage::disableItem()
{
    auto resources = selectedResources();
    auto indexes = m_model->indexesForResources(resources);
    m_model->setResourceEnabled(indexes, EnableAction::DISABLE);
}

void ModFolderPage::viewHomepage()
{
    for (auto resource : selectedResources()) {
        auto url = resource->homepage();
        if (!url.isEmpty()) {
            DesktopServices::openUrl(url);
        }
    }
}

void ModFolderPage::itemActivated(const QModelIndex&)
{
    auto resources = selectedResources();
    auto indexes = m_model->indexesForResources(resources);
    m_model->setResourceEnabled(indexes, EnableAction::TOGGLE);
}

bool ModFolderPage::eventFilter(QObject* obj, QEvent* ev)
{
    if (ev->type() == QEvent::KeyPress && obj == ui->treeView) {
        auto* keyEvent = static_cast<QKeyEvent*>(ev);
        switch (keyEvent->key()) {
            case Qt::Key_Delete:
                removeItem();
                return true;
            case Qt::Key_Plus:
                addItem();
                return true;
            default:
                break;
        }
    }
    return ExternalResourcesPage::eventFilter(obj, ev);
}

void ModFolderPage::downloadMods()
{
    if (m_instance->typeName() != "Minecraft") {
        return;  // this is a null instance or a legacy instance
    }

    auto profile = static_cast<MinecraftInstance*>(m_instance)->getPackProfile();
    if (!profile->getModLoaders().has_value()) {
        if (handleNoModLoader()) {
            return;
        }
    }

    m_downloadDialog = new ResourceDownload::ModDownloadDialog(this, m_model, m_instance);
    connect(this, &QObject::destroyed, m_downloadDialog, &QDialog::close);
    connect(m_downloadDialog, &QDialog::finished, this, &ModFolderPage::downloadDialogFinished);

    m_downloadDialog->setTargetDirectory(m_treeModel->targetDirForSelection(selectedSourceIndexes()));

    m_downloadDialog->open();
}

void ModFolderPage::downloadDialogFinished(int result)
{
    if (result) {
        auto tasks = new ConcurrentTask(tr("Download Mods"), APPLICATION->settings()->get("NumberOfConcurrentDownloads").toInt());
        connect(tasks, &Task::failed, [this, tasks](QString reason) {
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

    auto profile = static_cast<MinecraftInstance*>(m_instance)->getPackProfile();
    if (!profile->getModLoaders().has_value()) {
        if (handleNoModLoader()) {
            return;
        }
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
    auto selection = selectedSourceIndexes();
    const bool useAll = selection.isEmpty();
    auto modsList = resourcesForUpdateSelection(selection);
    if (modsList.isEmpty()) {
        CustomMessageBox::selectable(this, tr("Update checker"), tr("No mods selected for update."))->exec();
        return;
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
                message = tr("All root mods are up-to-date! :)");
            } else {
                message = tr("All selected mods are up-to-date! :)");
            }
        }
        CustomMessageBox::selectable(this, tr("Update checker"), message)->exec();
        return;
    }

    if (updateDialog.exec()) {
        auto tasks = new ConcurrentTask("Download Mods", APPLICATION->settings()->get("NumberOfConcurrentDownloads").toInt());
        connect(tasks, &Task::failed, [this, tasks](QString reason) {
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

        for (auto task : updateDialog.getTasks()) {
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
    auto resources = selectedResources();
    auto selectionCount = resources.length();
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

    auto indexes = m_model->indexesForResources(resources);
    m_model->deleteMetadata(indexes);
}

void ModFolderPage::changeModVersion()
{
    if (m_instance->typeName() != "Minecraft") {
        return;  // this is a null instance or a legacy instance
    }

    auto profile = static_cast<MinecraftInstance*>(m_instance)->getPackProfile();
    if (!profile->getModLoaders().has_value()) {
        if (handleNoModLoader()) {
            return;
        }
    }
    if (APPLICATION->settings()->get("ModMetadataDisabled").toBool()) {
        QMessageBox::critical(this, tr("Error"), tr("Mod updates are unavailable when metadata is disabled!"));
        return;
    }
    auto modsList = selectedMods();
    if (modsList.length() != 1 || modsList[0]->metadata() == nullptr) {
        return;
    }

    m_downloadDialog = new ResourceDownload::ModDownloadDialog(this, m_model, m_instance);
    connect(this, &QObject::destroyed, m_downloadDialog, &QDialog::close);
    connect(m_downloadDialog, &QDialog::finished, this, &ModFolderPage::downloadDialogFinished);

    m_downloadDialog->setTargetDirectory(QDir(modsList[0]->fileinfo().absolutePath()));
    m_downloadDialog->setResourceMetadata((*modsList.begin())->metadata());
    m_downloadDialog->open();
}

void ModFolderPage::exportModMetadata()
{
    auto mods = selectedMods();
    if (mods.isEmpty()) {
        mods = m_model->allMods();
    }
    std::sort(mods.begin(), mods.end(), [](const Mod* a, const Mod* b) { return a->name() < b->name(); });
    ExportToModListDialog dlg(m_instance->name(), mods, this);
    dlg.exec();
}

void ModFolderPage::createFolder()
{
    if (!m_instance || m_instance->typeName() != "Minecraft") {
        return;
    }

    bool ok = false;
    auto targetDir = m_treeModel->targetDirForSelection(selectedSourceIndexes());

    QString name =
        QInputDialog::getText(this, tr("New Folder"), tr("Enter a new folder name."), QLineEdit::Normal, tr("New Folder"), &ok).trimmed();
    if (!ok) {
        return;
    }

    name = FS::RemoveInvalidFilenameChars(name, '-');
    if (name.isEmpty()) {
        return;
    }

    auto folderName = FS::DirNameFromString(name, targetDir.absolutePath());
    if (folderName.isEmpty()) {
        CustomMessageBox::selectable(this, tr("Error"), tr("Failed to create folder. Please choose a different name."),
                                     QMessageBox::Critical)
            ->show();
        return;
    }

    if (!targetDir.mkpath(folderName)) {
        CustomMessageBox::selectable(this, tr("Error"), tr("Failed to create folder '%1'.").arg(folderName), QMessageBox::Critical)
            ->show();
        return;
    }

    m_model->update();
}

CoreModFolderPage::CoreModFolderPage(BaseInstance* inst, std::shared_ptr<ModFolderModel> mods, QWidget* parent)
    : ModFolderPage(inst, mods, parent)
{
    auto mcInst = dynamic_cast<MinecraftInstance*>(m_instance);
    if (mcInst) {
        auto version = mcInst->getPackProfile();
        if (version && version->getComponent("net.minecraftforge") && version->getComponent("net.minecraft")) {
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
                    update->start();
                }
            }
        }
    }
}

bool CoreModFolderPage::shouldDisplay() const
{
    if (ModFolderPage::shouldDisplay()) {
        auto inst = dynamic_cast<MinecraftInstance*>(m_instance);
        if (!inst) {
            return true;
        }

        auto version = inst->getPackProfile();
        if (!version || !version->getComponent("net.minecraftforge") || !version->getComponent("net.minecraft")) {
            return false;
        }
        auto minecraftCmp = version->getComponent("net.minecraft");
        return minecraftCmp->m_loaded && minecraftCmp->getReleaseDateTime() < g_VersionFilterData.legacyCutoffDate;
    }
    return false;
}

NilModFolderPage::NilModFolderPage(BaseInstance* inst, std::shared_ptr<ModFolderModel> mods, QWidget* parent)
    : ModFolderPage(inst, mods, parent)
{}

bool NilModFolderPage::shouldDisplay() const
{
    return m_model->dir().exists();
}

// Helper function so this doesn't need to be duplicated 3 times
inline bool ModFolderPage::handleNoModLoader()
{
    int resp =
        QMessageBox::question(this, this->tr("Missing Mod Loader"),
                              this->tr("You need to install a compatible mod loader before installing mods. Would you like to do so?"),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    switch (resp) {
        case QMessageBox::Yes: {
            // Should be safe
            auto profile = static_cast<MinecraftInstance*>(this->m_instance)->getPackProfile();
            InstallLoaderDialog dialog(profile, QString(), this);
            bool ret = dialog.exec();
            this->m_container->refreshContainer();

            // returning negation of dialog.exec which'll be true if the install loader dialog got canceled/closed
            // and false if the user went through and installed a loader
            return !ret;
        }
        case QMessageBox::No: {
            // Nothing happens the dialog is already closing
            // returning true so the caller doesn't go and continue with opening it's dialog without a mod loader
            return true;
        }
        default: {
            // Unreachable
            // returning true as a safety measure
            return true;
        }
    }
}
