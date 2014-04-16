/* Copyright 2014 MultiMC Contributors
 *
 * Authors:
 *      Taeyeon Mori <orochimarufan.x3@gmail.com>
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

#include <QtWidgets/QDialog>
#include <QtCore/QEventLoop>

#include "logic/auth/MojangAccount.h"

namespace Ui
{
class LoginDialog;
}

class LoginDialog : public QDialog
{
	Q_OBJECT

public:
	~LoginDialog();

	static MojangAccountPtr newAccount(QWidget *parent, QString message);

private:
	explicit LoginDialog(QWidget *parent = 0);

	void setUserInputsEnabled(bool enable);

protected
slots:
	void accept();
	void reject();

	void onTaskFailed(QString);
	void onTaskSucceeded();
	void onTaskStatus(QString);
	void onTaskProgress(qint64, qint64);

	void on_userTextBox_textEdited(QString);
	void on_passTextBox_textEdited(QString);

private:
	Ui::LoginDialog *ui;
	QEventLoop loop;
};
