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

#pragma once

#include <QDialog>

class ProgressProvider;

namespace Ui
{
class ProgressDialog;
}

class ProgressDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ProgressDialog(QWidget *parent = 0);
	~ProgressDialog();

	void updateSize();

	int exec(ProgressProvider *task);

	ProgressProvider *getTask();

public
slots:
	void onTaskStarted();
	void onTaskFailed(QString failure);
	void onTaskSucceeded();

	void changeStatus(const QString &status);
	void changeProgress(qint64 current, qint64 total);

signals:

protected:
	virtual void keyPressEvent(QKeyEvent *e);
	virtual void closeEvent(QCloseEvent *e);

private:
	Ui::ProgressDialog *ui;

	ProgressProvider *task;
};
