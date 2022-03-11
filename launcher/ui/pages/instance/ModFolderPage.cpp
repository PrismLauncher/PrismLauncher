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

#include "DesktopServices.h"

#include "minecraft/PackProfile.h"
#include "minecraft/VersionFilterData.h"
#include "minecraft/mod/Mod.h"
#include "minecraft/mod/ModFolderModel.h"

#include "modplatform/ModAPI.h"

#include "Version.h"
#include "tasks/SequentialTask.h"
#include "ui/dialogs/ProgressDialog.h"

ModFolderPage::ModFolderPage(BaseInstance* inst, std::shared_ptr<ModFolderModel> mods, QWidget* parent)
    : ExternalResourcesPage(inst, mods, parent)
{
    // This is structured like that so that these changes
    // do not affect the Resource pack and Shader pack tabs
    {
        auto act = new QAction(tr("Download mods"), this);
        act->setToolTip(tr("Download mods from online mod platforms"));
        ui->actionsToolbar->insertActionBefore(ui->actionAddItem, act);
        connect(act, &QAction::triggered, this, &ModFolderPage::installMods);

        ui->actionAddItem->setText(tr("Add .jar"));
        ui->actionAddItem->setToolTip(tr("Add mods via local file"));
    }
}

CoreModFolderPage::CoreModFolderPage(BaseInstance* inst, std::shared_ptr<ModFolderModel> mods, QWidget* parent)
    : ModFolderPage(inst, mods, parent)
{}

bool ModFolderPage::shouldDisplay() const
{
    return true;
}

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
        SequentialTask* tasks = new SequentialTask(this);
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
