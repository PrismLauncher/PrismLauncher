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

#pragma once

#include <QDialog>
#include <QRegularExpressionValidator>

QT_BEGIN_NAMESPACE
namespace Ui {
class ChooseOfflineNameDialog;
}
QT_END_NAMESPACE

class ChooseOfflineNameDialog final : public QDialog {
    Q_OBJECT

   public:
    explicit ChooseOfflineNameDialog(const QString& message, QWidget* parent = nullptr);
    ~ChooseOfflineNameDialog() override;

    QString getUsername() const;
    void setUsername(const QString& username) const;

   private:
    void updateAcceptAllowed(const QString& username) const;

   protected slots:
    void on_usernameTextBox_textEdited(const QString& newText) const;
    void on_allowInvalidUsernames_checkStateChanged(Qt::CheckState checkState) const;

   private:
    Ui::ChooseOfflineNameDialog* ui;
    QRegularExpressionValidator* m_usernameValidator;
};
