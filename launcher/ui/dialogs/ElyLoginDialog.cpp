// SPDX-License-Identifier: GPL-3.0-only
/*
 *  ElyPrismLauncher - Minecraft Launcher
 *  Copyright (c) 2025 Octol1ttle <l1ttleofficial@outlook.com>
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

#include "ElyLoginDialog.h"

#include "ui_MSALoginDialog.h"

MinecraftAccountPtr ElyLoginDialog::newAccount(QWidget* parent)
{
    ElyLoginDialog dlg(parent);
    if (dlg.exec() == QDialog::Accepted) {
        return dlg.m_account;
    }
    return nullptr;
}

ElyLoginDialog::ElyLoginDialog(QWidget* parent) : MSALoginDialog(parent)
{
    m_accountType = AccountType::Ely;
    m_linkUrl = "http://account.ely.by/code";

    setWindowTitle(tr("Add Ely.by account"));
    ui->loginButton->setText(tr("Sign in with Ely.by"));
}
