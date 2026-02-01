// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2022 TheKodeToad <TheKodeToad@proton.me>
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

#include "WorldListPage.h"
#include "minecraft/WorldList.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui_WorldListPage.h"

#include <ui/widgets/PageContainer.h>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QEvent>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QTreeView>
#include <Qt>

#include "FileSystem.h"
#include "tools/MCEditTool.h"

#include "DesktopServices.h"
#include "ui/GuiUtil.h"

#include "Application.h"
#include "DataPackPage.h"

class WorldListProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

   public:
    WorldListProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {}

    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const
    {
        QModelIndex sourceIndex = mapToSource(index);

        if (index.column() == 0 && role == Qt::DecorationRole) {
            WorldList* worlds = qobject_cast<WorldList*>(sourceModel());
            auto iconFile = worlds->data(sourceIndex, WorldList::IconFileRole).toString();
            if (iconFile.isNull()) {
                // NOTE: Minecraft uses the same placeholder for servers AND worlds
                return QIcon::fromTheme("unknown_server");
            }
            return QIcon(iconFile);
        }

        return sourceIndex.data(role);
    }
};

WorldListPage::WorldListPage(MinecraftInstance* inst, WorldList* worlds, QWidget* parent)
    : QMainWindow(parent), m_inst(inst), ui(new Ui::WorldListPage), m_worlds(worlds)
{
    ui->setupUi(this);

    ui->toolBar->insertSpacer(ui->actionRefresh);

    WorldListProxyModel* proxy = new WorldListProxyModel(this);
    proxy->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxy->setSourceModel(m_worlds);
    proxy->setSortRole(Qt::UserRole);
    ui->worldTreeView->setSortingEnabled(true);
    ui->worldTreeView->setModel(proxy);
    ui->worldTreeView->installEventFilter(this);
    ui->worldTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->worldTreeView->setIconSize(QSize(64, 64));
    connect(ui->worldTreeView, &QTreeView::customContextMenuRequested, this, &WorldListPage::ShowContextMenu);

    auto head = ui->worldTreeView->header();
    head->setSectionResizeMode(0, QHeaderView::Stretch);
    head->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    head->setSectionResizeMode(4, QHeaderView::ResizeToContents);

    connect(ui->worldTreeView->selectionModel(), &QItemSelectionModel::currentChanged, this, &WorldListPage::worldChanged);
    worldChanged(QModelIndex(), QModelIndex());
}

void WorldListPage::openedImpl()
{
    m_worlds->startWatching();

    if (!m_inst || !m_inst->traits().contains("feature:is_quick_play_singleplayer")) {
        ui->toolBar->removeAction(ui->actionJoin);
    }

    auto const setting_name = QString("WideBarVisibility_%1").arg(id());
    m_wide_bar_setting = APPLICATION->settings()->getOrRegisterSetting(setting_name);

    ui->toolBar->setVisibilityState(QByteArray::fromBase64(m_wide_bar_setting->get().toString().toUtf8()));
}

void WorldListPage::closedImpl()
{
    m_worlds->stopWatching();

    m_wide_bar_setting->set(QString::fromUtf8(ui->toolBar->getVisibilityState().toBase64()));
}

WorldListPage::~WorldListPage()
{
    m_worlds->stopWatching();
    delete ui;
}

void WorldListPage::ShowContextMenu(const QPoint& pos)
{
    auto menu = ui->toolBar->createContextMenu(this, tr("Context menu"));
    menu->exec(ui->worldTreeView->mapToGlobal(pos));
    delete menu;
}

QMenu* WorldListPage::createPopupMenu()
{
    QMenu* filteredMenu = QMainWindow::createPopupMenu();
    filteredMenu->removeAction(ui->toolBar->toggleViewAction());
    return filteredMenu;
}

bool WorldListPage::shouldDisplay() const
{
    return true;
}

void WorldListPage::retranslate()
{
    ui->retranslateUi(this);
}

bool WorldListPage::worldListFilter(QKeyEvent* keyEvent)
{
    if (keyEvent->key() == Qt::Key_Delete) {
        on_actionRemove_triggered();
        return true;
    }
    return QWidget::eventFilter(ui->worldTreeView, keyEvent);
}

bool WorldListPage::eventFilter(QObject* obj, QEvent* ev)
{
    if (ev->type() != QEvent::KeyPress) {
        return QWidget::eventFilter(obj, ev);
    }
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(ev);
    if (obj == ui->worldTreeView)
        return worldListFilter(keyEvent);
    return QWidget::eventFilter(obj, ev);
}

void WorldListPage::on_actionRemove_triggered()
{
    auto proxiedIndex = getSelectedWorld();

    if (!proxiedIndex.isValid())
        return;

    auto result = CustomMessageBox::selectable(this, tr("Confirm Deletion"),
                                               tr("You are about to delete \"%1\".\n"
                                                  "The world may be gone forever (A LONG TIME).\n\n"
                                                  "Are you sure?")
                                                   .arg(m_worlds->allWorlds().at(proxiedIndex.row()).name()),
                                               QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
                      ->exec();

    if (result != QMessageBox::Yes) {
        return;
    }
    m_worlds->stopWatching();
    m_worlds->deleteWorld(proxiedIndex.row());
    m_worlds->startWatching();
}

void WorldListPage::on_actionView_Folder_triggered()
{
    DesktopServices::openPath(m_worlds->dir().absolutePath(), true);
}

void WorldListPage::on_actionData_Packs_triggered()
{
    QModelIndex index = getSelectedWorld();

    if (!index.isValid()) {
        return;
    }

    if (!worldSafetyNagQuestion(tr("Manage Data Packs")))
        return;

    const QString fullPath = m_worlds->data(index, WorldList::FolderRole).toString();
    const QString folder = FS::PathCombine(fullPath, "datapacks");

    auto dialog = new QDialog(this);
    dialog->setWindowTitle(tr("Data packs for %1").arg(m_worlds->data(index, WorldList::NameRole).toString()));
    dialog->setWindowModality(Qt::WindowModal);

    dialog->resize(static_cast<int>(std::max(0.5 * window()->width(), 400.0)),
                   static_cast<int>(std::max(0.75 * window()->height(), 400.0)));
    dialog->restoreGeometry(QByteArray::fromBase64(APPLICATION->settings()->get("DataPackDownloadGeometry").toByteArray()));

    GenericPageProvider provider(dialog->windowTitle());

    bool isIndexed = !APPLICATION->settings()->get("ModMetadataDisabled").toBool();
    m_datapackModel.reset(new DataPackFolderModel(folder, m_inst, isIndexed, true));

    provider.addPageCreator([this] { return new DataPackPage(m_inst, m_datapackModel.get(), this); });

    auto layout = new QVBoxLayout(dialog);

    auto focusStealer = new QPushButton(dialog);
    layout->addWidget(focusStealer);
    focusStealer->setDefault(true);
    focusStealer->hide();

    auto pageContainer = new PageContainer(&provider, {}, dialog);
    pageContainer->hidePageList();
    layout->addWidget(pageContainer);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Close | QDialogButtonBox::Help);
    connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    connect(buttonBox, &QDialogButtonBox::helpRequested, pageContainer, &PageContainer::help);
    layout->addWidget(buttonBox);

    dialog->setLayout(layout);

    dialog->exec();

    APPLICATION->settings()->set("DataPackDownloadGeometry", dialog->saveGeometry().toBase64());
}

void WorldListPage::on_actionReset_Icon_triggered()
{
    auto proxiedIndex = getSelectedWorld();

    if (!proxiedIndex.isValid())
        return;

    if (m_worlds->resetIcon(proxiedIndex.row())) {
        ui->actionReset_Icon->setEnabled(false);
    }
}

QModelIndex WorldListPage::getSelectedWorld()
{
    auto index = ui->worldTreeView->selectionModel()->currentIndex();

    auto proxy = (QSortFilterProxyModel*)ui->worldTreeView->model();
    return proxy->mapToSource(index);
}

void WorldListPage::on_actionCopy_Seed_triggered()
{
    QModelIndex index = getSelectedWorld();

    if (!index.isValid()) {
        return;
    }
    int64_t seed = m_worlds->data(index, WorldList::SeedRole).toLongLong();
    APPLICATION->clipboard()->setText(QString::number(seed));
}

void WorldListPage::on_actionMCEdit_triggered()
{
    if (m_mceditStarting)
        return;

    auto mcedit = APPLICATION->mcedit();

    const QString mceditPath = mcedit->path();

    QModelIndex index = getSelectedWorld();

    if (!index.isValid()) {
        return;
    }

    if (!worldSafetyNagQuestion(tr("Open World in MCEdit")))
        return;

    auto fullPath = m_worlds->data(index, WorldList::FolderRole).toString();

    auto program = mcedit->getProgramPath();
    if (program.size()) {
#ifdef Q_OS_WIN32
        if (!QProcess::startDetached(program, { fullPath }, mceditPath)) {
            mceditError();
        }
#else
        m_mceditProcess.reset(new LoggedProcess());
        m_mceditProcess->setDetachable(true);
        connect(m_mceditProcess.get(), &LoggedProcess::stateChanged, this, &WorldListPage::mceditState);
        m_mceditProcess->start(program, { fullPath });
        m_mceditProcess->setWorkingDirectory(mceditPath);
        m_mceditStarting = true;
#endif
    } else {
        QMessageBox::warning(this->parentWidget(), tr("No MCEdit found or set up!"),
                             tr("You do not have MCEdit set up or it was moved.\nYou can set it up in the global settings."));
    }
}

void WorldListPage::mceditError()
{
    QMessageBox::warning(this->parentWidget(), tr("MCEdit failed to start!"),
                         tr("MCEdit failed to start.\nIt may be necessary to reinstall it."));
}

void WorldListPage::mceditState(LoggedProcess::State state)
{
    bool failed = false;
    switch (state) {
        case LoggedProcess::NotRunning:
        case LoggedProcess::Starting:
            return;
        case LoggedProcess::FailedToStart:
        case LoggedProcess::Crashed:
        case LoggedProcess::Aborted: {
            failed = true;
        }
        /* fallthrough */
        case LoggedProcess::Running:
        case LoggedProcess::Finished: {
            m_mceditStarting = false;
            break;
        }
    }
    if (failed) {
        mceditError();
    }
}

void WorldListPage::worldChanged([[maybe_unused]] const QModelIndex& current, [[maybe_unused]] const QModelIndex& previous)
{
    QModelIndex index = getSelectedWorld();
    bool enable = index.isValid();
    ui->actionCopy_Seed->setEnabled(enable);
    ui->actionMCEdit->setEnabled(enable);
    ui->actionRemove->setEnabled(enable);
    ui->actionCopy->setEnabled(enable);
    ui->actionRename->setEnabled(enable);
    ui->actionData_Packs->setEnabled(enable);
    bool hasIcon = !index.data(WorldList::IconFileRole).isNull();
    ui->actionReset_Icon->setEnabled(enable && hasIcon);

    auto supportsJoin = m_inst && m_inst->traits().contains("feature:is_quick_play_singleplayer");
    ui->actionJoin->setEnabled(enable && supportsJoin);

    if (!supportsJoin) {
        ui->toolBar->removeAction(ui->actionJoin);
    }
}

void WorldListPage::on_actionAdd_triggered()
{
    auto list = GuiUtil::BrowseForFiles(displayName(), tr("Select a Minecraft world zip"), tr("Minecraft World Zip File") + " (*.zip)",
                                        QString(), this->parentWidget());
    if (!list.empty()) {
        m_worlds->stopWatching();
        for (auto filename : list) {
            m_worlds->installWorld(QFileInfo(filename));
        }
        m_worlds->startWatching();
    }
}

bool WorldListPage::isWorldSafe(QModelIndex)
{
    return !m_inst->isRunning();
}

bool WorldListPage::worldSafetyNagQuestion(const QString& actionType)
{
    if (!isWorldSafe(getSelectedWorld())) {
        auto result = QMessageBox::question(
            this, actionType, tr("Changing a world while Minecraft is running is potentially unsafe.\nDo you wish to proceed?"));
        if (result == QMessageBox::No) {
            return false;
        }
    }
    return true;
}

void WorldListPage::on_actionCopy_triggered()
{
    QModelIndex index = getSelectedWorld();
    if (!index.isValid()) {
        return;
    }

    if (!worldSafetyNagQuestion(tr("Copy World")))
        return;

    auto worldVariant = m_worlds->data(index, WorldList::ObjectRole);
    auto world = (World*)worldVariant.value<void*>();
    bool ok = false;
    QString name =
        QInputDialog::getText(this, tr("World name"), tr("Enter a new name for the copy."), QLineEdit::Normal, world->name(), &ok);

    if (ok && name.length() > 0) {
        world->install(m_worlds->dir().absolutePath(), name);
    }
}

void WorldListPage::on_actionRename_triggered()
{
    QModelIndex index = getSelectedWorld();
    if (!index.isValid()) {
        return;
    }

    if (!worldSafetyNagQuestion(tr("Rename World")))
        return;

    auto worldVariant = m_worlds->data(index, WorldList::ObjectRole);
    auto world = (World*)worldVariant.value<void*>();

    bool ok = false;
    QString name = QInputDialog::getText(this, tr("World name"), tr("Enter a new world name."), QLineEdit::Normal, world->name(), &ok);

    if (ok && name.length() > 0) {
        world->rename(name);
    }
}

void WorldListPage::on_actionRefresh_triggered()
{
    m_worlds->update();
}

void WorldListPage::on_actionJoin_triggered()
{
    QModelIndex index = getSelectedWorld();
    if (!index.isValid()) {
        return;
    }
    auto worldVariant = m_worlds->data(index, WorldList::ObjectRole);
    auto world = (World*)worldVariant.value<void*>();
    APPLICATION->launch(m_inst, true, false, std::make_shared<MinecraftTarget>(MinecraftTarget::parse(world->folderName(), true)));
}

#include "WorldListPage.moc"
