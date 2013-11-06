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

#include "ProgressDialog.h"
#include "ui_ProgressDialog.h"

#include <QKeyEvent>

#include "logic/tasks/Task.h"
#include "gui/Platform.h"

ProgressDialog::ProgressDialog(QWidget *parent) : QDialog(parent), ui(new Ui::ProgressDialog)
{
	MultiMCPlatform::fixWM_CLASS(this);
	ui->setupUi(this);
	updateSize();

	changeProgress(0, 100);
}

ProgressDialog::~ProgressDialog()
{
	delete ui;
}

void ProgressDialog::updateSize()
{
	resize(QSize(480, minimumSizeHint().height()));
}

int ProgressDialog::exec(ProgressProvider *task)
{
	this->task = task;

	// Connect signals.
	connect(task, SIGNAL(started()), SLOT(onTaskStarted()));
	connect(task, SIGNAL(failed(QString)), SLOT(onTaskFailed(QString)));
	connect(task, SIGNAL(succeeded()), SLOT(onTaskSucceeded()));
	connect(task, SIGNAL(status(QString)), SLOT(changeStatus(const QString &)));
	connect(task, SIGNAL(progress(qint64, qint64)), SLOT(changeProgress(qint64, qint64)));

	// this makes sure that the task is started after the dialog is created
	QMetaObject::invokeMethod(task, "start", Qt::QueuedConnection);
	return QDialog::exec();
}

ProgressProvider *ProgressDialog::getTask()
{
	return task;
}

void ProgressDialog::onTaskStarted()
{
}

void ProgressDialog::onTaskFailed(QString failure)
{
	reject();
}

void ProgressDialog::onTaskSucceeded()
{
	accept();
}

void ProgressDialog::changeStatus(const QString &status)
{
	ui->statusLabel->setText(status);
	updateSize();
}

void ProgressDialog::changeProgress(qint64 current, qint64 total)
{
	ui->taskProgressBar->setMaximum(total);
	ui->taskProgressBar->setValue(current);
}

void ProgressDialog::keyPressEvent(QKeyEvent *e)
{
	if (e->key() == Qt::Key_Escape)
		return;
	QDialog::keyPressEvent(e);
}

void ProgressDialog::closeEvent(QCloseEvent *e)
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
