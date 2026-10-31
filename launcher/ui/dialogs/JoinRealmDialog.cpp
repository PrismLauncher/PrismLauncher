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

#include "JoinRealmDialog.h"
#include "ui_JoinRealmDialog.h"

#include <QPushButton>

#include "minecraft/Realms.h"

JoinRealmDialog::JoinRealmDialog(MinecraftInstance* instance, QWidget* parent) : QDialog(parent), ui(new Ui::JoinRealmDialog)
{
    ui->setupUi(this);
    ui->realmsView->header()->setSectionResizeMode(0, QHeaderView::Stretch);

    auto* joinButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    joinButton->setText(tr("Join"));
    joinButton->setEnabled(false);
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(ui->realmsView, &QTreeWidget::itemSelectionChanged, this,
            [this, joinButton] { joinButton->setEnabled(!ui->realmsView->selectedItems().isEmpty()); });
    connect(ui->realmsView, &QTreeWidget::itemActivated, this, &QDialog::accept);

    m_fetchJob = Realms::fetch(instance, this, [this](const QList<Realms::Realm>& realms) {
        for (const auto& realm : realms) {
            auto status = realm.expired ? tr("Expired") : realm.open ? tr("Open") : tr("Closed");
            auto* item = new QTreeWidgetItem(ui->realmsView, { realm.name, realm.owner, status, realm.motd });
            item->setData(0, Qt::UserRole, realm.id);
        }
        if (realms.isEmpty()) {
            ui->infoLabel->setText(tr("You don't have access to any Realms."));
        } else {
            ui->infoLabel->hide();
        }
    });
    if (!m_fetchJob) {
        ui->infoLabel->setText(tr("A Microsoft account that owns Minecraft is required to join Realms."));
        return;
    }
    connect(m_fetchJob.get(), &Task::failed, this,
            [this](const QString& reason) { ui->infoLabel->setText(tr("Failed to load Realms: %1").arg(reason)); });
}

JoinRealmDialog::~JoinRealmDialog()
{
    delete ui;
}

QString JoinRealmDialog::selectedRealmId() const
{
    auto items = ui->realmsView->selectedItems();
    return items.isEmpty() ? QString() : items.first()->data(0, Qt::UserRole).toString();
}
