/* Copyright 2013 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "taskdialog.h"
#include "ui_taskdialog.h"

#include <QKeyEvent>

#include "logic/tasks/Task.h"

TaskDialog::TaskDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::TaskDialog)
{
	ui->setupUi(this);
	updateSize();
	
	changeProgress(0);
}

TaskDialog::~TaskDialog()
{
	delete ui;
}

void TaskDialog::updateSize()
{
	resize(QSize(480, minimumSizeHint().height()));
}

void TaskDialog::exec(Task *task)
{
	this->task = task;
	
	// Connect signals.
	connect(task, SIGNAL(started()), SLOT(onTaskStarted()));
	connect(task, SIGNAL(failed(QString)), SLOT(onTaskEnded()));
	connect(task, SIGNAL(succeeded()), SLOT(onTaskEnded()));
	connect(task, SIGNAL(statusChanged(const QString&)), SLOT(changeStatus(const QString&)));
	connect(task, SIGNAL(progressChanged(int)), SLOT(changeProgress(int)));
	
	// this makes sure that the task is started after the dialog is created
	QMetaObject::invokeMethod(task, "startTask", Qt::QueuedConnection);
	QDialog::exec();
}

Task* TaskDialog::getTask()
{
	return task;
}

void TaskDialog::onTaskStarted()
{
	
}

void TaskDialog::onTaskEnded()
{
	close();
}

void TaskDialog::changeStatus(const QString &status)
{
	ui->statusLabel->setText(status);
	updateSize();
}

void TaskDialog::changeProgress(int progress)
{
	if (progress < 0)
		progress = 0;
	else if (progress > 100)
		progress = 100;
	
	ui->taskProgressBar->setValue(progress);
}

void TaskDialog::keyPressEvent(QKeyEvent* e)
{
	if (e->key() == Qt::Key_Escape)
		return;
	QDialog::keyPressEvent(e);
}

void TaskDialog::closeEvent(QCloseEvent* e)
{
	if (task && task->isRunning())
	{
		e->ignore();
	}
	else
	{
		QDialog::closeEvent(e);
	}
}
