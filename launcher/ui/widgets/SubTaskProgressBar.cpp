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

#include "SubTaskProgressBar.h"
#include "tasks/Task.h"
#include "ui_SubTaskProgressBar.h"

SubTaskProgressBar::SubTaskProgressBar(QWidget* parent) : QWidget(parent), ui(new Ui::SubTaskProgressBar)
{
    ui->setupUi(this);
    ui->taskProgressScrollArea->hide();
}

SubTaskProgressBar::~SubTaskProgressBar()
{
    delete ui;
}

void SubTaskProgressBar::addSubTask(TaskV2* subTask)
{
    auto p = new SubTaskProgressBar(this);
    p->setTask(subTask);
    ui->taskProgressLayout->addWidget(p);
    m_running_subtasks++;
    ui->taskProgressScrollArea->show();
    connect(subTask, &TaskV2::finished, this, [this] {
        m_running_subtasks--;
        if (!m_running_subtasks) {
            ui->taskProgressScrollArea->hide();
        }
    });
}

void SubTaskProgressBar::setTask(TaskV2* t)
{
    if (m_task) {
        disconnect(m_task, nullptr, this, nullptr);
    }
    m_task = t;
    connect(t, &TaskV2::finished, [this] { hide(); });
    connect(t, &TaskV2::started, [this] { show(); });
    connect(t, &TaskV2::totalChanged, [this](TaskV2* job, double total, double delta) { ui->progressBar->setRange(0, total); });
    connect(t, &TaskV2::processedChanged, [this](TaskV2* job, double current, double delta) { ui->progressBar->setValue(current); });
    connect(t, &TaskV2::stateChanged, [this](TaskV2* job) {
        ui->statusLabel->setText(job->status());
        ui->statusDetailsLabel->setText(job->details());
    });
    connect(t, &TaskV2::subTaskAdded, [this](TaskV2* job, TaskV2* subTask) { addSubTask(subTask); });
    for (auto subTask : t->subTasks()) {
        addSubTask(subTask.get());
    }
    if (t->isRunning()) {
        ui->statusLabel->setText(t->status());
        ui->statusDetailsLabel->setText(t->details());
        ui->progressBar->setRange(0, t->progressTotal());
        ui->progressBar->setValue(t->progress());
    }
}
