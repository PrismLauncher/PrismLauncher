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

#pragma once

#include <QDialog>

namespace Ui
{
class LWJGLSelectDialog;
}

class LWJGLSelectDialog : public QDialog
{
	Q_OBJECT

public:
	explicit LWJGLSelectDialog(QWidget *parent = 0);
	~LWJGLSelectDialog();

	QString selectedVersion() const;

private
slots:
	void on_refreshButton_clicked();

	void loadingStateUpdated(bool loading);
	void loadingFailed(QString error);

private:
	Ui::LWJGLSelectDialog *ui;
};
