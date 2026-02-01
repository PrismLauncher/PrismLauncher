// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
 *  Copyright (C) 2025 Yihe Li <winmikedows@hotmail.com>
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

#include <QLayout>
#include <QPushButton>

#include "BuildConfig.h"
#include "CreateShortcutDialog.h"
#include "ui_CreateShortcutDialog.h"

#include "ui/dialogs/IconPickerDialog.h"

#include "BaseInstance.h"
#include "DesktopServices.h"
#include "FileSystem.h"
#include "InstanceList.h"
#include "icons/IconList.h"

#include "minecraft/MinecraftInstance.h"
#include "minecraft/ShortcutUtils.h"
#include "minecraft/WorldList.h"
#include "minecraft/auth/AccountList.h"

CreateShortcutDialog::CreateShortcutDialog(BaseInstance* instance, QWidget* parent)
    : QDialog(parent), ui(new Ui::CreateShortcutDialog), m_instance(instance)
{
    ui->setupUi(this);

    InstIconKey = instance->iconKey();
    ui->iconButton->setIcon(APPLICATION->icons()->getIcon(InstIconKey));
    ui->instNameTextBox->setPlaceholderText(instance->name());

    auto mInst = dynamic_cast<MinecraftInstance*>(instance);
    m_QuickJoinSupported = mInst && mInst->traits().contains("feature:is_quick_play_singleplayer");
    auto worldList = mInst->worldList();
    worldList->update();
    if (!m_QuickJoinSupported || worldList->empty()) {
        ui->worldTarget->hide();
        ui->worldSelectionBox->hide();
        ui->serverTarget->setChecked(true);
        ui->serverTarget->hide();
        ui->serverLabel->show();
    }

    // Populate save targets
    if (!DesktopServices::isFlatpak()) {
        QString desktopDir = FS::getDesktopDir();
        QString applicationDir = FS::getApplicationsDir();

        if (!desktopDir.isEmpty())
            ui->saveTargetSelectionBox->addItem(tr("Desktop"), QVariant::fromValue(ShortcutTarget::Desktop));

        if (!applicationDir.isEmpty())
            ui->saveTargetSelectionBox->addItem(tr("Applications"), QVariant::fromValue(ShortcutTarget::Applications));
    }
    ui->saveTargetSelectionBox->addItem(tr("Other..."), QVariant::fromValue(ShortcutTarget::Other));

    // Populate worlds
    if (m_QuickJoinSupported) {
        for (const auto& world : worldList->allWorlds()) {
            // Entry name: World Name [Game Mode] - Last Played: DateTime
            QString entry_name = tr("%1 [%2] - Last Played: %3")
                                     .arg(world.name(), world.gameType().toTranslatedString(), world.lastPlayed().toString(Qt::ISODate));
            ui->worldSelectionBox->addItem(entry_name, world.name());
        }
    }

    // Populate accounts
    auto accounts = APPLICATION->accounts();
    MinecraftAccountPtr defaultAccount = accounts->defaultAccount();
    if (accounts->count() <= 0) {
        ui->overrideAccountCheckbox->setEnabled(false);
    } else {
        for (int i = 0; i < accounts->count(); i++) {
            MinecraftAccountPtr account = accounts->at(i);
            auto profileLabel = account->profileName();
            if (account->isInUse())
                profileLabel = tr("%1 (in use)").arg(profileLabel);
            auto face = account->getFace();
            QIcon icon = face.isNull() ? QIcon::fromTheme("noaccount") : face;
            ui->accountSelectionBox->addItem(profileLabel, account->profileName());
            ui->accountSelectionBox->setItemIcon(i, icon);
            if (defaultAccount == account)
                ui->accountSelectionBox->setCurrentIndex(i);
        }
    }
}

CreateShortcutDialog::~CreateShortcutDialog()
{
    delete ui;
}

void CreateShortcutDialog::on_iconButton_clicked()
{
    IconPickerDialog dlg(this);
    dlg.execWithSelection(InstIconKey);

    if (dlg.result() == QDialog::Accepted) {
        InstIconKey = dlg.selectedIconKey;
        ui->iconButton->setIcon(APPLICATION->icons()->getIcon(InstIconKey));
    }
}

void CreateShortcutDialog::on_overrideAccountCheckbox_stateChanged(int state)
{
    ui->accountOptionsGroup->setEnabled(state == Qt::Checked);
}

void CreateShortcutDialog::on_targetCheckbox_stateChanged(int state)
{
    ui->targetOptionsGroup->setEnabled(state == Qt::Checked);
    ui->worldSelectionBox->setEnabled(ui->worldTarget->isChecked());
    ui->serverAddressBox->setEnabled(ui->serverTarget->isChecked());
    stateChanged();
}

void CreateShortcutDialog::on_worldTarget_toggled(bool checked)
{
    ui->worldSelectionBox->setEnabled(checked);
    stateChanged();
}

void CreateShortcutDialog::on_serverTarget_toggled(bool checked)
{
    ui->serverAddressBox->setEnabled(checked);
    stateChanged();
}

void CreateShortcutDialog::on_worldSelectionBox_currentIndexChanged(int index)
{
    stateChanged();
}

void CreateShortcutDialog::on_serverAddressBox_textChanged(const QString& text)
{
    stateChanged();
}

void CreateShortcutDialog::stateChanged()
{
    QString result = m_instance->name();
    if (ui->targetCheckbox->isChecked()) {
        if (ui->worldTarget->isChecked())
            result = tr("%1 - %2").arg(result, ui->worldSelectionBox->currentData().toString());
        else if (ui->serverTarget->isChecked())
            result = tr("%1 - Server %2").arg(result, ui->serverAddressBox->text());
    }
    ui->instNameTextBox->setPlaceholderText(result);
    if (!ui->targetCheckbox->isChecked())
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    else {
        ui->buttonBox->button(QDialogButtonBox::Ok)
            ->setEnabled((ui->worldTarget->isChecked() && ui->worldSelectionBox->currentIndex() != -1) ||
                         (ui->serverTarget->isChecked() && !ui->serverAddressBox->text().isEmpty()));
    }
}

// Real work
void CreateShortcutDialog::createShortcut()
{
    QString targetString = tr("instance");
    QStringList extraArgs;
    if (ui->targetCheckbox->isChecked()) {
        if (ui->worldTarget->isChecked()) {
            targetString = tr("world");
            extraArgs = { "--world", ui->worldSelectionBox->currentData().toString() };
        } else if (ui->serverTarget->isChecked()) {
            targetString = tr("server");
            extraArgs = { "--server", ui->serverAddressBox->text() };
        }
    }

    auto target = ui->saveTargetSelectionBox->currentData().value<ShortcutTarget>();
    auto name = ui->instNameTextBox->text();
    if (name.isEmpty())
        name = ui->instNameTextBox->placeholderText();
    if (ui->overrideAccountCheckbox->isChecked())
        extraArgs.append({ "--profile", ui->accountSelectionBox->currentData().toString() });

    ShortcutUtils::Shortcut args{ m_instance, name, targetString, this, extraArgs, InstIconKey, target };
    if (target == ShortcutTarget::Desktop)
        ShortcutUtils::createInstanceShortcutOnDesktop(args);
    else if (target == ShortcutTarget::Applications)
        ShortcutUtils::createInstanceShortcutInApplications(args);
    else
        ShortcutUtils::createInstanceShortcutInOther(args);
}
