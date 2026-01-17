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
#include "minecraft/PackProfile.h"
#include "ui_ExternalResourcesPage.h"

#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ProgressDialog.h"
#include "ui/dialogs/ResourceDownloadDialog.h"
#include "ui/dialogs/ResourceUpdateDialog.h"

DataPackPage::DataPackPage(BaseInstance* instance, DataPackFolderModel* model, QWidget* parent)
    : ExternalResourcesPage(instance, model, parent), m_model(model)
{
    ui->actionDownloadItem->setText(tr("Download Packs"));
    ui->actionDownloadItem->setToolTip(tr("Download data packs from online mod platforms"));
    ui->actionDownloadItem->setEnabled(true);
    ui->actionsToolbar->insertActionBefore(ui->actionAddItem, ui->actionDownloadItem);

    connect(ui->actionDownloadItem, &QAction::triggered, this, &DataPackPage::downloadDataPacks);

    ui->actionUpdateItem->setToolTip(tr("Try to check or update all selected data packs (all data packs if none are selected)"));
    connect(ui->actionUpdateItem, &QAction::triggered, this, &DataPackPage::updateDataPacks);
    ui->actionsToolbar->insertActionBefore(ui->actionAddItem, ui->actionUpdateItem);

    auto updateMenu = new QMenu(this);

    auto update = updateMenu->addAction(ui->actionUpdateItem->text());
    connect(update, &QAction::triggered, this, &DataPackPage::updateDataPacks);

    updateMenu->addAction(ui->actionResetItemMetadata);
    connect(ui->actionResetItemMetadata, &QAction::triggered, this, &DataPackPage::deleteDataPackMetadata);

    ui->actionUpdateItem->setMenu(updateMenu);

    ui->actionChangeVersion->setToolTip(tr("Change a data pack's version."));
    connect(ui->actionChangeVersion, &QAction::triggered, this, &DataPackPage::changeDataPackVersion);
    ui->actionsToolbar->insertActionAfter(ui->actionUpdateItem, ui->actionChangeVersion);
}

void DataPackPage::updateFrame(const QModelIndex& current, [[maybe_unused]] const QModelIndex& previous)
{
    auto sourceCurrent = m_filterModel->mapToSource(current);
    int row = sourceCurrent.row();
    auto& dp = m_model->at(row);
    ui->frame->updateWithDataPack(dp);
}

void DataPackPage::downloadDataPacks()
{
    if (m_instance->typeName() != "Minecraft")
        return;  // this is a null instance or a legacy instance

    m_downloadDialog = new ResourceDownload::DataPackDownloadDialog(this, m_model, m_instance);
    connect(this, &QObject::destroyed, m_downloadDialog, &QDialog::close);
    connect(m_downloadDialog, &QDialog::finished, this, &DataPackPage::downloadDialogFinished);

    m_downloadDialog->open();
}

void DataPackPage::downloadDialogFinished(int result)
{
    if (result) {
        auto tasks = new ConcurrentTask(tr("Download Data Packs"), APPLICATION->settings()->get("NumberOfConcurrentDownloads").toInt());
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
    if (m_downloadDialog)
        m_downloadDialog->deleteLater();
}

void DataPackPage::updateDataPacks()
{
    if (m_instance->typeName() != "Minecraft")
        return;  // this is a null instance or a legacy instance

    if (APPLICATION->settings()->get("ModMetadataDisabled").toBool()) {
        QMessageBox::critical(this, tr("Error"), tr("Data pack updates are unavailable when metadata is disabled!"));
        return;
    }
    if (m_instance != nullptr && m_instance->isRunning()) {
        auto response =
            CustomMessageBox::selectable(this, tr("Confirm Update"),
                                         tr("Updating data packs while the game is running may cause pack duplication and game crashes.\n"
                                            "The old files may not be deleted as they are in use.\n"
                                            "Are you sure you want to do this?"),
                                         QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
                ->exec();

        if (response != QMessageBox::Yes)
            return;
    }
    auto selection = m_filterModel->mapSelectionToSource(ui->treeView->selectionModel()->selection()).indexes();

    auto mods_list = m_model->selectedResources(selection);
    bool use_all = mods_list.empty();
    if (use_all)
        mods_list = m_model->allResources();

    ResourceUpdateDialog update_dialog(this, m_instance, m_model, mods_list, false, { ModPlatform::ModLoaderType::DataPack });
    update_dialog.checkCandidates();

    if (update_dialog.aborted()) {
        CustomMessageBox::selectable(this, tr("Aborted"), tr("The data pack updater was aborted!"), QMessageBox::Warning)->show();
        return;
    }
    if (update_dialog.noUpdates()) {
        QString message{ tr("'%1' is up-to-date! :)").arg(mods_list.front()->name()) };
        if (mods_list.size() > 1) {
            if (use_all) {
                message = tr("All data packs are up-to-date! :)");
            } else {
                message = tr("All selected data packs are up-to-date! :)");
            }
        }
        CustomMessageBox::selectable(this, tr("Update checker"), message)->exec();
        return;
    }

    if (update_dialog.exec()) {
        auto tasks = new ConcurrentTask("Download Data Packs", APPLICATION->settings()->get("NumberOfConcurrentDownloads").toInt());
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

void DataPackPage::deleteDataPackMetadata()
{
    auto selection = m_filterModel->mapSelectionToSource(ui->treeView->selectionModel()->selection()).indexes();
    auto selectionCount = m_model->selectedDataPacks(selection).length();
    if (selectionCount == 0)
        return;
    if (selectionCount > 1) {
        auto response = CustomMessageBox::selectable(this, tr("Confirm Removal"),
                                                     tr("You are about to remove the metadata for %1 data packs.\n"
                                                        "Are you sure?")
                                                         .arg(selectionCount),
                                                     QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
                            ->exec();

        if (response != QMessageBox::Yes)
            return;
    }

    m_model->deleteMetadata(selection);
}

void DataPackPage::changeDataPackVersion()
{
    if (m_instance->typeName() != "Minecraft")
        return;  // this is a null instance or a legacy instance

    if (APPLICATION->settings()->get("ModMetadataDisabled").toBool()) {
        QMessageBox::critical(this, tr("Error"), tr("Data pack updates are unavailable when metadata is disabled!"));
        return;
    }

    const QModelIndexList rows = ui->treeView->selectionModel()->selectedRows();

    if (rows.count() != 1)
        return;

    Resource& resource = m_model->at(m_filterModel->mapToSource(rows[0]).row());

    if (resource.metadata() == nullptr)
        return;

    ResourceDownload::DataPackDownloadDialog mdownload(this, m_model, m_instance);
    mdownload.setResourceMetadata(resource.metadata());
    if (mdownload.exec()) {
        auto tasks = new ConcurrentTask("Download Data Packs", APPLICATION->settings()->get("NumberOfConcurrentDownloads").toInt());
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

GlobalDataPackPage::GlobalDataPackPage(MinecraftInstance* instance, QWidget* parent) : QWidget(parent), m_instance(instance)
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    connect(instance->settings()->getSetting("GlobalDataPacksEnabled").get(), &Setting::SettingChanged, this, [this] {
        updateContent();
        if (m_container != nullptr)
            m_container->refreshContainer();
    });

    connect(instance->settings()->getSetting("GlobalDataPacksPath").get(), &Setting::SettingChanged, this,
            &GlobalDataPackPage::updateContent);
}

QString GlobalDataPackPage::displayName() const
{
    if (m_underlyingPage == nullptr)
        return {};

    return m_underlyingPage->displayName();
}

QIcon GlobalDataPackPage::icon() const
{
    if (m_underlyingPage == nullptr)
        return {};

    return m_underlyingPage->icon();
}

QString GlobalDataPackPage::helpPage() const
{
    if (m_underlyingPage == nullptr)
        return {};

    return m_underlyingPage->helpPage();
}

bool GlobalDataPackPage::shouldDisplay() const
{
    return m_instance->settings()->get("GlobalDataPacksEnabled").toBool();
}

bool GlobalDataPackPage::apply()
{
    return m_underlyingPage == nullptr || m_underlyingPage->apply();
}

void GlobalDataPackPage::openedImpl()
{
    if (m_underlyingPage != nullptr)
        m_underlyingPage->openedImpl();
}

void GlobalDataPackPage::closedImpl()
{
    if (m_underlyingPage != nullptr)
        m_underlyingPage->closedImpl();
}

void GlobalDataPackPage::updateContent()
{
    if (m_underlyingPage != nullptr) {
        if (m_container->selectedPage() == this)
            m_underlyingPage->closedImpl();

        m_underlyingPage->apply();

        layout()->removeWidget(m_underlyingPage);

        delete m_underlyingPage;
        m_underlyingPage = nullptr;
    }

    if (shouldDisplay()) {
        m_underlyingPage = new DataPackPage(m_instance, m_instance->dataPackList());
        m_underlyingPage->setParentContainer(m_container);
        m_underlyingPage->updateExtraInfo = [this](QString id, QString value) { updateExtraInfo(std::move(id), std::move(value)); };

        if (m_container->selectedPage() == this)
            m_underlyingPage->openedImpl();

        layout()->addWidget(m_underlyingPage);
    }
}
void GlobalDataPackPage::setParentContainer(BasePageContainer* container)
{
    BasePage::setParentContainer(container);
    updateContent();
}
