/// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PrismLaucher - Minecraft Launcher
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
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "ProgressDialog.h"
#include "ui_ProgressDialog.h"

#include <limits>
#include <QDebug>
#include <QKeyEvent>

#include "tasks/Task.h"

#include "ui/widgets/SubTaskProgressBar.h"


template<typename T> 
int map_int_range(T value)
{
    // auto type_min = std::numeric_limits<T>::min();
    auto type_min = 0;
    auto type_max = std::numeric_limits<T>::max();

    // auto int_min = std::numeric_limits<int>::min();
    auto int_min = 0;
    auto int_max = std::numeric_limits<int>::max();

    auto type_range = type_max - type_min;
    auto int_range = int_max - int_min;

    return static_cast<int>((value - type_min) * int_range / type_range + int_min);
}


ProgressDialog::ProgressDialog(QWidget* parent) : QDialog(parent), ui(new Ui::ProgressDialog)
{
    ui->setupUi(this);
    ui->taskProgressScrollArea->setHidden(true);
    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setAttribute(Qt::WidgetAttribute::WA_QuitOnClose, true);
    setSkipButton(false);
    changeProgress(0, 100);
}

void ProgressDialog::setSkipButton(bool present, QString label)
{
    ui->skipButton->setAutoDefault(false);
    ui->skipButton->setDefault(false);
    ui->skipButton->setFocusPolicy(Qt::ClickFocus);
    ui->skipButton->setEnabled(present);
    ui->skipButton->setVisible(present);
    ui->skipButton->setText(label);
    updateSize();
}

void ProgressDialog::on_skipButton_clicked(bool checked)
{
    Q_UNUSED(checked);
    if (ui->skipButton->isEnabled())  // prevent other triggers from aborting
        task->abort();
}

ProgressDialog::~ProgressDialog()
{
    delete ui;
}

void ProgressDialog::updateSize()
{   
    QSize lastSize = this->size();
    QSize qSize = QSize(480, minimumSizeHint().height());
    resize(qSize);
    setFixedSize(qSize);
    // keep the dialog in the center after a resize
    if (lastSize != qSize)
        this->move(
            this->parentWidget()->x() + (this->parentWidget()->width() - this->width()) / 2,
            this->parentWidget()->y() + (this->parentWidget()->height() - this->height()) / 2
        );
}

int ProgressDialog::execWithTask(Task* task)
{
    this->task = task;

    if (!task) {
        qDebug() << "Programmer error: Progress dialog created with null task.";
        return QDialog::DialogCode::Accepted;
    }

    QDialog::DialogCode result {};
    if (handleImmediateResult(result)) {
        return result;
    }

    // Connect signals.
    connect(task, &Task::started, this, &ProgressDialog::onTaskStarted);
    connect(task, &Task::failed, this, &ProgressDialog::onTaskFailed);
    connect(task, &Task::succeeded, this, &ProgressDialog::onTaskSucceeded);
    connect(task, &Task::status, this, &ProgressDialog::changeStatus);
    connect(task, &Task::stepProgress, this, &ProgressDialog::changeStepProgress);
    connect(task, &Task::progress, this, &ProgressDialog::changeProgress);

    connect(task, &Task::aborted, this, &ProgressDialog::hide);
    connect(task, &Task::abortStatusChanged, ui->skipButton, &QPushButton::setEnabled);

    m_is_multi_step = task->isMultiStep();
    ui->taskProgressScrollArea->setHidden(!m_is_multi_step);
    updateSize();

    // It's a good idea to start the task after we entered the dialog's event loop :^)
    if (!task->isRunning()) {
        QMetaObject::invokeMethod(task, &Task::start, Qt::QueuedConnection);
    } else {
        changeStatus(task->getStatus());
        changeProgress(task->getProgress(), task->getTotalProgress());
    }

    // auto size_hint = ui->verticalLayout->sizeHint();
    // resize(size_hint.width(), size_hint.height());

    return QDialog::exec();
}

// TODO: only provide the unique_ptr overloads
int ProgressDialog::execWithTask(std::unique_ptr<Task>&& task)
{
    connect(this, &ProgressDialog::destroyed, task.get(), &Task::deleteLater);
    return execWithTask(task.release());
}
int ProgressDialog::execWithTask(std::unique_ptr<Task>& task)
{
    connect(this, &ProgressDialog::destroyed, task.get(), &Task::deleteLater);
    return execWithTask(task.release());
}

bool ProgressDialog::handleImmediateResult(QDialog::DialogCode& result)
{
    if (task->isFinished()) {
        if (task->wasSuccessful()) {
            result = QDialog::Accepted;
        } else {
            result = QDialog::Rejected;
        }
        return true;
    }
    return false;
}

Task* ProgressDialog::getTask()
{
    return task;
}

void ProgressDialog::onTaskStarted() {}

void ProgressDialog::onTaskFailed(QString failure)
{
    reject();
    hide();
}

void ProgressDialog::onTaskSucceeded()
{
    accept();
    hide();
}

void ProgressDialog::changeStatus(const QString& status)
{
    ui->globalStatusLabel->setText(task->getStatus());
    ui->globalStatusDetailsLabel->setText(task->getDetails());

    updateSize();
}

void ProgressDialog::addTaskProgress(TaskStepProgress* progress)
{
    SubTaskProgressBar* task_bar = new SubTaskProgressBar(this);
    taskProgress.insert(progress->uid, task_bar);
    ui->taskProgressLayout->insertWidget(0, task_bar);
}

void ProgressDialog::changeStepProgress(TaskStepProgressList task_progress)
{
    m_is_multi_step = true;
    ui->taskProgressScrollArea->setHidden(false);
    
    for (auto tp : task_progress) {
        if (!taskProgress.contains(tp->uid))
            addTaskProgress(tp.get());
        auto task_bar = taskProgress.value(tp->uid);

        if (tp->total < 0) {
            task_bar->setRange(0, 0);
        } else {
            task_bar->setRange(0, map_int_range<qint64>(tp->total));
        }

        task_bar->setValue(map_int_range<qint64>(tp->current));
        task_bar->setStatus(tp->status);
        task_bar->setDetails(tp->details);

        if (tp->isDone()) {
            task_bar->setVisible(false);
        }

    }

    updateSize();
}

void ProgressDialog::changeProgress(qint64 current, qint64 total)
{
    ui->globalProgressBar->setMaximum(total);
    ui->globalProgressBar->setValue(current);

}

void ProgressDialog::keyPressEvent(QKeyEvent* e)
{
    if (ui->skipButton->isVisible()) {
        if (e->key() == Qt::Key_Escape) {
            on_skipButton_clicked(true);
            return;
        } else if (e->key() == Qt::Key_Tab) {
            ui->skipButton->setFocusPolicy(Qt::StrongFocus);
            ui->skipButton->setFocus();
            ui->skipButton->setAutoDefault(true);
            ui->skipButton->setDefault(true);
            return;
        }
    }
    QDialog::keyPressEvent(e);
}

void ProgressDialog::closeEvent(QCloseEvent* e)
{
    if (task && task->isRunning()) {
        e->ignore();
    } else {
        QDialog::closeEvent(e);
    }
}
