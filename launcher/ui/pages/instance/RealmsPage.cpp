// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Vishrut Sachan <vishrutsachan2004@gmail.com>
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

#include "RealmsPage.h"
#include "ui_RealmsPage.h"

#include <QMenu>

#include "Application.h"
#include "minecraft/MinecraftInstance.h"

RealmsPage::RealmsPage(MinecraftInstance* inst, QWidget* parent) : QMainWindow(parent), ui(new Ui::RealmsPage), m_inst(inst)
{
    ui->setupUi(this);
    ui->realmsView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->realmsView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->realmsView, &QTreeWidget::customContextMenuRequested, this, &RealmsPage::ShowContextMenu);
    connect(ui->realmsView, &QTreeWidget::itemSelectionChanged, this, &RealmsPage::updateState);
    connect(ui->realmsView, &QTreeWidget::itemActivated, this, [this] {
        if (ui->actionJoin->isEnabled()) {
            on_actionJoin_triggered();
        }
    });
    connect(m_inst, &MinecraftInstance::runningStatusChanged, this, &RealmsPage::updateState);
    updateState();
}

RealmsPage::~RealmsPage()
{
    delete ui;
}

bool RealmsPage::shouldDisplay() const
{
    return m_inst->traits().contains("feature:is_quick_play_singleplayer");
}

void RealmsPage::openedImpl()
{
    on_actionRefresh_triggered();
}

void RealmsPage::retranslate()
{
    ui->retranslateUi(this);
}

void RealmsPage::ShowContextMenu(const QPoint& pos)
{
    auto menu = ui->toolBar->createContextMenu(this, tr("Context menu"));
    menu->exec(ui->realmsView->mapToGlobal(pos));
    delete menu;
}

QMenu* RealmsPage::createPopupMenu()
{
    QMenu* filteredMenu = QMainWindow::createPopupMenu();
    filteredMenu->removeAction(ui->toolBar->toggleViewAction());
    return filteredMenu;
}

void RealmsPage::updateState()
{
    auto* item = ui->realmsView->currentItem();
    ui->actionJoin->setEnabled(item && item->isSelected() && !m_inst->isRunning());
}

void RealmsPage::on_actionJoin_triggered()
{
    auto realmId = ui->realmsView->currentItem()->data(0, Qt::UserRole).toString();
    APPLICATION->launch(m_inst, LaunchMode::Normal, std::make_shared<MinecraftTarget>(MinecraftTarget::fromRealm(realmId)));
}

void RealmsPage::on_actionRefresh_triggered()
{
    m_fetchJob = Realms::fetch(m_inst, this, [this](const QList<Realms::Realm>& realms) {
        ui->realmsView->clear();
        for (const auto& realm : realms) {
            auto status = realm.expired ? tr("Expired") : realm.open ? tr("Open") : tr("Closed");
            auto* item = new QTreeWidgetItem(ui->realmsView, { realm.name, realm.owner, status, realm.motd });
            item->setData(0, Qt::UserRole, realm.id);
        }
        ui->infoLabel->setVisible(realms.isEmpty());
        ui->infoLabel->setText(tr("You don't have access to any Realms."));
        updateState();
    });
    if (!m_fetchJob) {
        ui->infoLabel->setVisible(true);
        ui->infoLabel->setText(tr("A Microsoft account that owns Minecraft is required to join Realms."));
        return;
    }
    connect(m_fetchJob.get(), &Task::failed, this, [this](const QString& reason) {
        ui->infoLabel->setVisible(true);
        ui->infoLabel->setText(tr("Failed to load Realms: %1").arg(reason));
    });
}
