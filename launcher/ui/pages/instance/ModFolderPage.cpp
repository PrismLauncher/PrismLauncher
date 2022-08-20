// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
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

#include "ModFolderPage.h"
#include "ui_ExternalResourcesPage.h"

#include <QAbstractItemModel>
#include <QEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QSortFilterProxyModel>

#include "Application.h"

#include "ui/GuiUtil.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ModDownloadDialog.h"
#include "ui/dialogs/ModUpdateDialog.h"

#include "DesktopServices.h"

#include "minecraft/PackProfile.h"
#include "minecraft/VersionFilterData.h"
#include "minecraft/mod/Mod.h"
#include "minecraft/mod/ModFolderModel.h"

#include "modplatform/ModAPI.h"

#include "Version.h"
#include "tasks/ConcurrentTask.h"
#include "ui/dialogs/ProgressDialog.h"

ModFolderPage::ModFolderPage(BaseInstance* inst, std::shared_ptr<ModFolderModel> mods, QWidget* parent)
    : ExternalResourcesPage(inst, mods, parent)
{
    // This is structured like that so that these changes
    // do not affect the Resource pack and Shader pack tabs
    {
        ui->actionDownloadItem->setText(tr("Download mods"));
        ui->actionDownloadItem->setToolTip(tr("Download mods from online mod platforms"));
        ui->actionDownloadItem->setEnabled(true);
        ui->actionAddItem->setText(tr("Add file"));
        ui->actionAddItem->setToolTip(tr("Add a locally downloaded file"));

        ui->actionsToolbar->insertActionBefore(ui->actionAddItem, ui->actionDownloadItem);

        connect(ui->actionDownloadItem, &QAction::triggered, this, &ModFolderPage::installMods);

        ui->actionUpdateItem->setToolTip(tr("Try to check or update all selected mods (all mods if none are selected)"));
        ui->actionsToolbar->insertActionAfter(ui->actionAddItem, ui->actionUpdateItem);
        connect(ui->actionUpdateItem, &QAction::triggered, this, &ModFolderPage::updateMods);

        auto check_allow_update = [this] {
            return (!m_instance || !m_instance->isRunning()) &&
                   (ui->treeView->selectionModel()->hasSelection() || !m_model->empty());
        };

        connect(ui->treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this, check_allow_update] {
            ui->actionUpdateItem->setEnabled(check_allow_update());
        });

        connect(mods.get(), &ModFolderModel::rowsInserted, this, [this, check_allow_update] {
            ui->actionUpdateItem->setEnabled(check_allow_update());
        });

        connect(mods.get(), &ModFolderModel::rowsRemoved, this, [this, check_allow_update] {
            ui->actionUpdateItem->setEnabled(check_allow_update());
        });

        connect(mods.get(), &ModFolderModel::updateFinished, this, [this, check_allow_update, mods] {
            ui->actionUpdateItem->setEnabled(check_allow_update());

            // Prevent a weird crash when trying to open the mods page twice in a session o.O
            disconnect(mods.get(), &ModFolderModel::updateFinished, this, 0);
        });

        ModFolderPage::runningStateChanged(m_instance && m_instance->isRunning());
    }
}

void ModFolderPage::runningStateChanged(bool running)
{
    ExternalResourcesPage::runningStateChanged(running);
    ui->actionDownloadItem->setEnabled(!running);
    ui->actionUpdateItem->setEnabled(!running);
}

bool ModFolderPage::shouldDisplay() const
{
    return true;
}

void ModFolderPage::installMods()
{
    if (!m_controlsEnabled)
        return;
    if (m_instance->typeName() != "Minecraft")
        return;  // this is a null instance or a legacy instance

    auto profile = static_cast<MinecraftInstance*>(m_instance)->getPackProfile();
    if (profile->getModLoaders() == ModAPI::Unspecified) {
        QMessageBox::critical(this, tr("Error"), tr("Please install a mod loader first!"));
        return;
    }

    ModDownloadDialog mdownload(m_model, this, m_instance);
    if (mdownload.exec()) {
        ConcurrentTask* tasks = new ConcurrentTask(this);
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
            if (warnings.count())
                CustomMessageBox::selectable(this, tr("Warnings"), warnings.join('\n'), QMessageBox::Warning)->show();

            tasks->deleteLater();
        });

        for (auto& task : mdownload.getTasks()) {
            tasks->addTask(task);
        }

        ProgressDialog loadDialog(this);
        loadDialog.setSkipButton(true, tr("Abort"));
        loadDialog.execWithTask(tasks);

        m_model->update();
    }
}

void ModFolderPage::updateMods()
{
    auto selection = m_filterModel->mapSelectionToSource(ui->treeView->selectionModel()->selection()).indexes();

    auto mods_list = m_model->selectedMods(selection);
    bool use_all = mods_list.empty();
    if (use_all)
        mods_list = m_model->allMods();

    ModUpdateDialog update_dialog(this, m_instance, m_model, mods_list);
    update_dialog.checkCandidates();

    if (update_dialog.aborted()) {
        CustomMessageBox::selectable(this, tr("Aborted"), tr("The mod updater was aborted!"), QMessageBox::Warning)->show();
        return;
    }
    if (update_dialog.noUpdates()) {
        QString message{ tr("'%1' is up-to-date! :)").arg(mods_list.front()->name()) };
        if (mods_list.size() > 1) {
            if (use_all) {
                message = tr("All mods are up-to-date! :)");
            } else {
                message = tr("All selected mods are up-to-date! :)");
            }
        }
        CustomMessageBox::selectable(this, tr("Update checker"), message)
            ->exec();
        return;
    }

    if (update_dialog.exec()) {
        ConcurrentTask* tasks = new ConcurrentTask(this);
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

        for (auto task : update_dialog.getTasks()) {
            tasks->addTask(task);
        }

        ProgressDialog loadDialog(this);
        loadDialog.setSkipButton(true, tr("Abort"));
        loadDialog.execWithTask(tasks);

        m_model->update();
    }
}

CoreModFolderPage::CoreModFolderPage(BaseInstance* inst, std::shared_ptr<ModFolderModel> mods, QWidget* parent)
    : ModFolderPage(inst, mods, parent)
{}

bool CoreModFolderPage::shouldDisplay() const
{
    if (ModFolderPage::shouldDisplay()) {
        auto inst = dynamic_cast<MinecraftInstance*>(m_instance);
        if (!inst)
            return true;

        auto version = inst->getPackProfile();

        if (!version)
            return true;
        if (!version->getComponent("net.minecraftforge"))
            return false;
        if (!version->getComponent("net.minecraft"))
            return false;
        if (version->getComponent("net.minecraft")->getReleaseDateTime() < g_VersionFilterData.legacyCutoffDate)
            return true;
    }
    return false;
}
