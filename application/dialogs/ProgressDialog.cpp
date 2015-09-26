/* Copyright 2013-2015 MultiMC Contributors
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

#include "tasks/Task.h"

ProgressDialog::ProgressDialog(QWidget *parent) : QDialog(parent), ui(new Ui::ProgressDialog)
{
	ui->setupUi(this);
	this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
	setSkipButton(false);
	changeProgress(0, 100);
}

void ProgressDialog::setSkipButton(bool present, QString label)
{
	ui->skipButton->setEnabled(present);
	ui->skipButton->setVisible(present);
	ui->skipButton->setText(label);
	updateSize();
}

void ProgressDialog::on_skipButton_clicked(bool checked)
{
	Q_UNUSED(checked);
	task->abort();
}

ProgressDialog::~ProgressDialog()
{
	delete ui;
}

void ProgressDialog::updateSize()
{
	resize(QSize(480, minimumSizeHint().height()));
}

int ProgressDialog::execWithTask(Task *task)
{
	this->task = task;
	QDialog::DialogCode result;

	if(handleImmediateResult(result))
	{
		return result;
	}

	// Connect signals.
	connect(task, SIGNAL(started()), SLOT(onTaskStarted()));
	connect(task, SIGNAL(failed(QString)), SLOT(onTaskFailed(QString)));
	connect(task, SIGNAL(succeeded()), SLOT(onTaskSucceeded()));
	connect(task, SIGNAL(status(QString)), SLOT(changeStatus(const QString &)));
	connect(task, SIGNAL(progress(qint64, qint64)), SLOT(changeProgress(qint64, qint64)));

	// if this didn't connect to an already running task, invoke start
	if(!task->isRunning())
	{
		task->start();
	}
	if(task->isRunning())
	{
		changeProgress(task->getProgress(), task->getTotalProgress());
		changeStatus(task->getStatus());
		return QDialog::exec();
	}
	else if(handleImmediateResult(result))
	{
		return result;
	}
	else
	{
		return QDialog::Rejected;
	}
}

bool ProgressDialog::handleImmediateResult(QDialog::DialogCode &result)
{
	if(task->isFinished())
	{
		if(task->successful())
		{
			result = QDialog::Accepted;
		}
		else
		{
			result = QDialog::Rejected;
		}
		return true;
	}
	return false;
}

Task *ProgressDialog::getTask()
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
