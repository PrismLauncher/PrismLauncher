// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
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
 */

#include "DataPackPage.h"

#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ProgressDialog.h"
#include "ui/dialogs/ResourceDownloadDialog.h"

DataPackPage::DataPackPage(MinecraftInstance* instance, std::shared_ptr<DataPackFolderModel> model, QWidget* parent)
    : ExternalResourcesPage(instance, model, parent)
{
    ui->actionDownloadItem->setText(tr("Download packs"));
    ui->actionDownloadItem->setToolTip(tr("Download data packs from online platforms"));
    ui->actionDownloadItem->setEnabled(true);
    connect(ui->actionDownloadItem, &QAction::triggered, this, &DataPackPage::downloadDataPacks);
    ui->actionsToolbar->insertActionBefore(ui->actionAddItem, ui->actionDownloadItem);
    
    ui->actionViewConfigs->setVisible(false);
}

bool DataPackPage::onSelectionChanged(const QModelIndex& current, [[maybe_unused]] const QModelIndex& previous)
{
    auto sourceCurrent = m_filterModel->mapToSource(current);
    int row = sourceCurrent.row();
    auto& dp = static_cast<DataPack&>(m_model->at(row));
    ui->frame->updateWithDataPack(dp);

    return true;
}

void DataPackPage::downloadDataPacks()
{
    if (m_instance->typeName() != "Minecraft")
        return;  // this is a null instance or a legacy instance

    ResourceDownload::DataPackDownloadDialog mdownload(this, std::static_pointer_cast<DataPackFolderModel>(m_model), m_instance);
    if (mdownload.exec()) {
        auto tasks =
            new ConcurrentTask(this, "Download Data Pack", APPLICATION->settings()->get("NumberOfConcurrentDownloads").toInt());
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
