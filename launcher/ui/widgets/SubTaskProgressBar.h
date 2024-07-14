// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PrismLaucher - Minecraft Launcher
 *  Copyright (C) 2022 Rachel Powers <508861+Ryex@users.noreply.github.com>
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

#include <QWidget>
#include "tasks/Task.h"

namespace Ui {
class SubTaskProgressBar;
}

class SubTaskProgressBar : public QWidget {
    Q_OBJECT
   public:
    SubTaskProgressBar(QWidget* parent = nullptr);
    ~SubTaskProgressBar();

    void setTask(TaskV2* t);

   private slots:
    void addSubTask(TaskV2* subTask);

   private:
    Ui::SubTaskProgressBar* ui;
    TaskV2* m_task;
    int m_running_subtasks = 0;
};
