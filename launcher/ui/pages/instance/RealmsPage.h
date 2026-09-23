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

#include <QMainWindow>

#include "minecraft/Realms.h"
#include "ui/pages/BasePage.h"

namespace Ui {
class RealmsPage;
}

class MinecraftInstance;

class RealmsPage : public QMainWindow, public BasePage {
    Q_OBJECT

   public:
    explicit RealmsPage(MinecraftInstance* inst, QWidget* parent = nullptr);
    ~RealmsPage() override;

    QString displayName() const override { return tr("Realms"); }
    QIcon icon() const override { return QIcon::fromTheme("server"); }
    QString id() const override { return "realms"; }
    bool shouldDisplay() const override;
    void openedImpl() override;
    void retranslate() override;

   protected:
    QMenu* createPopupMenu() override;

   private:
    void updateState();

   private slots:
    void on_actionJoin_triggered();
    void on_actionRefresh_triggered();
    void ShowContextMenu(const QPoint& pos);

   private:
    Ui::RealmsPage* ui = nullptr;
    MinecraftInstance* m_inst = nullptr;
    NetJob::Ptr m_fetchJob;
};
