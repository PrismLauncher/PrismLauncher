// SPDX-FileCopyrightText: 2023 Rachel Powers <508861+Ryex@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 Rachel Powers <508861+Ryex@users.noreply.github.com>
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
 */
#pragma once

#include <QDialog>

namespace Ui {
class UpdateAvailableDialog;
}

class UpdateAvailableDialog : public QDialog {
    Q_OBJECT

   public:
    enum ResultCode {
        Install = 10,
        DontInstall = 11,
        Skip = 12,
    };

    explicit UpdateAvailableDialog(const QString& currentVersion,
                                   const QString& availableVersion,
                                   const QString& releaseNotes,
                                   QWidget* parent = 0);
    ~UpdateAvailableDialog() = default;

   private:
    Ui::UpdateAvailableDialog* ui;
};
