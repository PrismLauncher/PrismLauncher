// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2024 Trial97 <alexandru.tripon97@gmail.com>
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
#include "BaseVersion.h"

namespace Ui {
class JavaDownload;
}

class JavaDownload : public QDialog {
    Q_OBJECT

   public:
    explicit JavaDownload(QWidget* parent = 0);
    ~JavaDownload();

    void accept();

   public slots:
    void refresh();

   protected slots:
    void setSelectedVersion(BaseVersion::Ptr version);

   private:
    Ui::JavaDownload* ui;
};
