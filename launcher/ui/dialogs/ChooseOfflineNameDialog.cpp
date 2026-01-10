// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2025 Octol1ttle <l1ttleofficial@outlook.com>
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

#include "ChooseOfflineNameDialog.h"

#include <QPushButton>
#include <QRegularExpression>

#include "ui_ChooseOfflineNameDialog.h"

ChooseOfflineNameDialog::ChooseOfflineNameDialog(const QString& message, QWidget* parent)
    : QDialog(parent), ui(new Ui::ChooseOfflineNameDialog)
{
    ui->setupUi(this);
    ui->label->setText(message);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));

    const QRegularExpression usernameRegExp("^[A-Za-z0-9_]{3,16}$");
    m_usernameValidator = new QRegularExpressionValidator(usernameRegExp, this);
    ui->usernameTextBox->setValidator(m_usernameValidator);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

ChooseOfflineNameDialog::~ChooseOfflineNameDialog()
{
    delete ui;
}

QString ChooseOfflineNameDialog::getUsername() const
{
    return ui->usernameTextBox->text();
}

void ChooseOfflineNameDialog::setUsername(const QString& username) const
{
    ui->usernameTextBox->setText(username);
    updateAcceptAllowed(username);
}

void ChooseOfflineNameDialog::updateAcceptAllowed(const QString& username) const
{
    const bool allowed = ui->allowInvalidUsernames->isChecked() ? !username.isEmpty() : ui->usernameTextBox->hasAcceptableInput();
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(allowed);
}

void ChooseOfflineNameDialog::on_usernameTextBox_textEdited(const QString& newText) const
{
    updateAcceptAllowed(newText);
}

void ChooseOfflineNameDialog::on_allowInvalidUsernames_checkStateChanged(const Qt::CheckState checkState) const
{
    ui->usernameTextBox->setValidator(checkState == Qt::Checked ? nullptr : m_usernameValidator);
    updateAcceptAllowed(getUsername());
}
