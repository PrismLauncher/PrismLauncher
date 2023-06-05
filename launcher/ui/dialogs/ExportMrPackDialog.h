// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
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
#include "BaseInstance.h"
#include "FastFileIconProvider.h"
#include "FileIgnoreProxy.h"

namespace Ui {
class ExportMrPackDialog;
}

class ExportMrPackDialog : public QDialog {
    Q_OBJECT

   public:
    explicit ExportMrPackDialog(InstancePtr instance, QWidget* parent = nullptr);
    ~ExportMrPackDialog();

    void done(int result) override;
    void validate();

   private:
    const InstancePtr instance;
    Ui::ExportMrPackDialog* ui;
    FileIgnoreProxy* proxy;
    FastFileIconProvider icons;
};
