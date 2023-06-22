// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2023 Trial97 <alexandru.tripon97@gmail.com>
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
#include <QList>
#include "BaseInstance.h"
#include "minecraft/mod/Mod.h"

namespace Ui {
class ExportModsToStringDialog;
}

class ExportModsToStringDialog : public QDialog {
    Q_OBJECT

   public:
    explicit ExportModsToStringDialog(InstancePtr instance, QWidget* parent = nullptr);
    ~ExportModsToStringDialog();

   protected slots:
    void formatChanged(int index);
    void triggerImp();
    void trigger(int) { triggerImp(); };

   private:
    QList<Mod*> m_allMods;
    bool m_template_selected;
    Ui::ExportModsToStringDialog* ui;
};
