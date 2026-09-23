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

#pragma once

#include <QDialog>

#include "net/NetJob.h"

namespace Ui {
class JoinRealmDialog;
}

class MinecraftInstance;

class JoinRealmDialog final : public QDialog {
    Q_OBJECT

   public:
    explicit JoinRealmDialog(MinecraftInstance* instance, QWidget* parent = nullptr);
    ~JoinRealmDialog() override;

    QString selectedRealmId() const;

   private:
    Ui::JoinRealmDialog* ui;
    NetJob::Ptr m_fetchJob;
};
