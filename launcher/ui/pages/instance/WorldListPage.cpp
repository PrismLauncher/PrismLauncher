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
#include "AssertHelpers.h"
#include "Commandline.h"
#include "minecraft/WorldList.h"
#include "settings/SettingsObject.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ProgressDialog.h"
#include "ui_WorldListPage.h"

#include <ui/widgets/PageContainer.h>
#include <QClipboard>
#include <QCursor>
#include <QDialogButtonBox>
#include <QEvent>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QTreeView>
#include <Qt>
#include <memory>

#include "FileSystem.h"

#include "DesktopServices.h"
#include "Json.h"
#include "ui/GuiUtil.h"

#include "Application.h"
#include "DataPackPage.h"

namespace {
class WorldListProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

   public:
    explicit WorldListProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {}

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        QModelIndex sourceIndex = mapToSource(index);

        if (index.column() == 0 && role == Qt::DecorationRole) {
            auto* worlds = qobject_cast<WorldList*>(sourceModel());
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
}  // namespace

WorldListPage::WorldListPage(MinecraftInstance* inst, WorldList* worlds, QWidget* parent)
    : QMainWindow(parent), m_inst(inst), m_ui(new Ui::WorldListPage), m_worlds(worlds), m_worldToolsMenu(new QMenu(this))
{
    m_ui->setupUi(this);

    m_ui->toolBar->insertSpacer(m_ui->actionRefresh);

    auto* proxy = new WorldListProxyModel(this);
    proxy->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxy->setSourceModel(m_worlds);
    proxy->setSortRole(Qt::UserRole);
    m_ui->worldTreeView->setSortingEnabled(true);
    m_ui->worldTreeView->setModel(proxy);
    m_ui->worldTreeView->installEventFilter(this);
    m_ui->worldTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_ui->worldTreeView->setIconSize(QSize(64, 64));
    connect(m_ui->worldTreeView, &QTreeView::customContextMenuRequested, this, &WorldListPage::ShowContextMenu);
    connect(m_ui->worldTreeView, &QAbstractItemView::activated, this, [this] {
        if (m_ui->actionJoin->isEnabled()) {
            on_actionJoin_triggered();
        }
    });

    auto* head = m_ui->worldTreeView->header();
    head->setSectionResizeMode(0, QHeaderView::Stretch);
    head->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    head->setSectionResizeMode(4, QHeaderView::ResizeToContents);

    connect(m_ui->worldTreeView->selectionModel(), &QItemSelectionModel::currentChanged, this, &WorldListPage::worldChanged);

    m_ui->actionWorldTools->setMenu(m_worldToolsMenu);
    connect(m_ui->actionWorldTools, &QAction::triggered, this, [this] {
        if (getSelectedWorld().isValid() && !m_worldToolsMenu->isEmpty()) {
            m_worldToolsMenu->popup(QCursor::pos());
        }
    });
    connect(APPLICATION->settings()->getSetting("WorldTools").get(), &Setting::SettingChanged, this, [this] { populateWorldToolsMenu(); });

    worldChanged(QModelIndex(), QModelIndex());
}

void WorldListPage::openedImpl()
{
    m_worlds->startWatching();

    if ((m_inst == nullptr) || !m_inst->traits().contains("feature:is_quick_play_singleplayer")) {
        m_ui->toolBar->removeAction(m_ui->actionJoin);
    }

    const auto settingName = QString("WideBarVisibility_%1").arg(id());
    m_wideBarSetting = APPLICATION->settings()->getOrRegisterSetting(settingName);

    m_ui->toolBar->setVisibilityState(QByteArray::fromBase64(m_wideBarSetting->get().toString().toUtf8()));

    populateWorldToolsMenu();
}

void WorldListPage::closedImpl()
{
    m_worlds->stopWatching();

    m_wideBarSetting->set(QString::fromUtf8(m_ui->toolBar->getVisibilityState().toBase64()));
}

WorldListPage::~WorldListPage()
{
    m_worlds->stopWatching();
    delete m_ui;
}

void WorldListPage::ShowContextMenu(const QPoint& pos)
{
    auto* menu = m_ui->toolBar->createContextMenu(this, tr("Context menu"));
    menu->exec(m_ui->worldTreeView->mapToGlobal(pos));
    delete menu;
}
QMenu* WorldListPage::createPopupMenu()
{
    QMenu* filteredMenu = QMainWindow::createPopupMenu();
    filteredMenu->removeAction(m_ui->toolBar->toggleViewAction());
    return filteredMenu;
}

bool WorldListPage::shouldDisplay() const
{
    return true;
}

void WorldListPage::retranslate()
{
    m_ui->retranslateUi(this);
}

bool WorldListPage::worldListFilter(QKeyEvent* ev)
{
    if (ev->key() == Qt::Key_Delete) {
        on_actionRemove_triggered();
        return true;
    }
    return QWidget::eventFilter(m_ui->worldTreeView, ev);
}

bool WorldListPage::eventFilter(QObject* obj, QEvent* ev)
{
    if (ev->type() != QEvent::KeyPress) {
        return QWidget::eventFilter(obj, ev);
    }
    auto* keyEvent = static_cast<QKeyEvent*>(ev);
    if (obj == m_ui->worldTreeView) {
        return worldListFilter(keyEvent);
    }
    return QWidget::eventFilter(obj, ev);
}

void WorldListPage::on_actionRemove_triggered()
{
    auto proxiedIndex = getSelectedWorld();
    if (!proxiedIndex.isValid()) {
        return;
    }

    const auto& world = m_worlds->allWorlds().at(proxiedIndex.row());

    auto result = CustomMessageBox::selectable(this, tr("Confirm Deletion"),
                                               tr("You are about to delete \"%1\".\n"
                                                  "The world may be gone forever (A LONG TIME).\n\n"
                                                  "Are you sure?")
                                                   .arg(world.name()),
                                               QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
                      ->exec();

    if (result != QMessageBox::Yes) {
        return;
    }

    auto task = m_worlds->createDeleteWorldTask(proxiedIndex.row());
    if (!task) {
        return;
    }

    m_worlds->stopWatching();

    ProgressDialog dialog(this);
    dialog.execWithTask(std::move(task));

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

    if (!worldSafetyNagQuestion(tr("Manage Data Packs"))) {
        return;
    }

    const QString fullPath = m_worlds->data(index, WorldList::FolderRole).toString();
    const QString folder = FS::PathCombine(fullPath, "datapacks");

    auto* dialog = new QDialog(this);
    dialog->setWindowTitle(tr("Data packs for %1").arg(m_worlds->data(index, WorldList::NameRole).toString()));
    dialog->setWindowModality(Qt::WindowModal);

    dialog->resize(static_cast<int>(std::max(0.5 * window()->width(), 400.0)),
                   static_cast<int>(std::max(0.75 * window()->height(), 400.0)));
    dialog->restoreGeometry(QByteArray::fromBase64(APPLICATION->settings()->get("DataPackDownloadGeometry").toByteArray()));

    GenericPageProvider provider(dialog->windowTitle());

    bool isIndexed = !APPLICATION->settings()->get("ModMetadataDisabled").toBool();
    m_datapackModel = std::make_unique<DataPackFolderModel>(folder, m_inst, isIndexed, true);

    provider.addPageCreator([this] { return new DataPackPage(m_inst, m_datapackModel.get(), this); });

    auto* layout = new QVBoxLayout(dialog);

    auto* focusStealer = new QPushButton(dialog);
    layout->addWidget(focusStealer);
    focusStealer->setDefault(true);
    focusStealer->hide();

    auto* pageContainer = new PageContainer(&provider, {}, dialog);
    pageContainer->hidePageList();
    layout->addWidget(pageContainer);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close | QDialogButtonBox::Help);
    connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    connect(buttonBox, &QDialogButtonBox::helpRequested, pageContainer, &PageContainer::help);
    layout->addWidget(buttonBox);

    dialog->setLayout(layout);

    dialog->setAttribute(Qt::WA_DeleteOnClose);

    connect(dialog, &QDialog::finished, this,
            [dialog] { APPLICATION->settings()->set("DataPackDownloadGeometry", dialog->saveGeometry().toBase64()); });

    dialog->open();
}

void WorldListPage::on_actionReset_Icon_triggered()
{
    auto proxiedIndex = getSelectedWorld();

    if (!proxiedIndex.isValid()) {
        return;
    }

    if (m_worlds->resetIcon(proxiedIndex.row())) {
        m_ui->actionReset_Icon->setEnabled(false);
    }
}

QModelIndex WorldListPage::getSelectedWorld()
{
    auto index = m_ui->worldTreeView->selectionModel()->currentIndex();

    auto* proxy = dynamic_cast<QSortFilterProxyModel*>(m_ui->worldTreeView->model());
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

void WorldListPage::populateWorldToolsMenu()
{
    m_worldToolsMenu->clear();
    const QVariantMap tools = Json::toMap(APPLICATION->settings()->get("WorldTools").toString());

    if (tools.isEmpty()) {
        auto* noToolsAction = m_worldToolsMenu->addAction(tr("No Tools Added"));
        noToolsAction->setEnabled(false);
        m_worldToolsMenu->addSeparator();
        auto* settingsAction = m_worldToolsMenu->addAction(tr("Open Settings"));
        connect(settingsAction, &QAction::triggered, this, [] { APPLICATION->ShowGlobalSettings(nullptr, "external-tools"); });
    } else {
        for (auto it = tools.constBegin(); it != tools.constEnd(); ++it) {
            if (ASSERT_NEVER(it.key().isEmpty())) {
                continue;
            }
            auto* action = m_worldToolsMenu->addAction(it.key());
            connect(action, &QAction::triggered, this,
                    [this, name = it.key(), command = it.value().toString()] { launchWorldTool(name, command); });
        }
    }

    m_ui->actionWorldTools->setEnabled(getSelectedWorld().isValid());
}

void WorldListPage::launchWorldTool(const QString& name, const QString& command)
{
    const QModelIndex index = getSelectedWorld();
    if (!index.isValid()) {
        return;
    }

    if (!worldSafetyNagQuestion(name)) {
        return;
    }

    const auto folderPath = m_worlds->data(index, WorldList::FolderRole).toString();
    QProcessEnvironment vars;
    vars.insert("WORLD_PATH", folderPath);

    auto args = Commandline::process(command, vars);
    if (args.isEmpty()) {
        QMessageBox::warning(this->parentWidget(), tr("Invalid command"), tr("The tool command is empty."));
        return;
    }

    const auto program = args.takeFirst();
    auto* process = new QProcess(this);
    process->setWorkingDirectory(folderPath);
    process->start(program, args);
    if (!process->waitForStarted()) {
        QMessageBox::warning(this->parentWidget(), tr("Tool failed to start!"),
                             tr("The tool could not be started.\nError: %1").arg(process->errorString()));
        process->deleteLater();
    }
}

void WorldListPage::worldChanged([[maybe_unused]] const QModelIndex& current, [[maybe_unused]] const QModelIndex& previous)
{
    QModelIndex index = getSelectedWorld();
    bool enable = index.isValid();
    m_ui->actionCopy_Seed->setEnabled(enable);
    m_ui->actionRemove->setEnabled(enable);
    m_ui->actionCopy->setEnabled(enable);
    m_ui->actionRename->setEnabled(enable);
    m_ui->actionData_Packs->setEnabled(enable);
    m_ui->actionWorldTools->setEnabled(enable);
    bool hasIcon = !index.data(WorldList::IconFileRole).isNull();
    m_ui->actionReset_Icon->setEnabled(enable && hasIcon);

    auto supportsJoin = (m_inst != nullptr) && m_inst->traits().contains("feature:is_quick_play_singleplayer");
    m_ui->actionJoin->setEnabled(enable && supportsJoin);

    if (!supportsJoin) {
        m_ui->toolBar->removeAction(m_ui->actionJoin);
    }
}

void WorldListPage::on_actionAdd_triggered()
{
    auto list = GuiUtil::BrowseForFiles(displayName(), tr("Select a Minecraft world zip"), tr("Minecraft World Zip File") + " (*.zip)",
                                        QString(), this->parentWidget());
    if (list.empty()) {
        return;
    }

    m_worlds->stopWatching();
    for (const auto& filename : list) {
        auto task = m_worlds->createInstallWorldTask(QFileInfo(filename));
        if (!task) {
            continue;
        }
        ProgressDialog dialog(this);
        dialog.execWithTask(std::move(task));
    }
    m_worlds->startWatching();
}

bool WorldListPage::isWorldSafe(QModelIndex /*unused*/)
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

    if (!worldSafetyNagQuestion(tr("Copy World"))) {
        return;
    }

    const auto world = m_worlds->allWorlds().at(index.row());

    bool ok = false;
    QString name =
        QInputDialog::getText(this, tr("World name"), tr("Enter a new name for the copy."), QLineEdit::Normal, world.name(), &ok);

    if (!ok || name.isEmpty()) {
        return;
    }

    auto task = m_worlds->createCopyWorldTask(index.row(), name);
    if (!task) {
        return;
    }

    m_worlds->stopWatching();

    ProgressDialog dialog(this);
    dialog.execWithTask(std::move(task));

    m_worlds->startWatching();
}

void WorldListPage::on_actionRename_triggered()
{
    QModelIndex index = getSelectedWorld();
    if (!index.isValid()) {
        return;
    }

    if (!worldSafetyNagQuestion(tr("Rename World"))) {
        return;
    }

    auto worldVariant = m_worlds->data(index, WorldList::ObjectRole);
    auto* world = static_cast<World*>(worldVariant.value<void*>());

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
    auto* world = static_cast<World*>(worldVariant.value<void*>());
    APPLICATION->launch(m_inst, LaunchMode::Normal, std::make_shared<MinecraftTarget>(MinecraftTarget::parse(world->folderName(), true)));
}

#include "WorldListPage.moc"
