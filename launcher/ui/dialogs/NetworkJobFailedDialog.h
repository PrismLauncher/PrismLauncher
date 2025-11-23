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

QT_BEGIN_NAMESPACE
namespace Ui {
class NetworkJobFailedDialog;
}
QT_END_NAMESPACE

class NetworkJobFailedDialog : public QDialog {
    Q_OBJECT

   public:
    explicit NetworkJobFailedDialog(QString jobName, int attempts, int requests, int failed, QWidget* parent = nullptr);
    ~NetworkJobFailedDialog() override;

    void addFailedRequest(QUrl url, QString error) const;

   private:
    Ui::NetworkJobFailedDialog* m_ui;
};
