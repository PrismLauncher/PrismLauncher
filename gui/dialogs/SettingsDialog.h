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

#include <memory>
#include <QDialog>

#include "logic/JavaChecker.h"

class SettingsObject;

namespace Ui
{
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
	Q_OBJECT

public:
	explicit SettingsDialog(QWidget *parent = 0);
	~SettingsDialog();

	void updateCheckboxStuff();

	void applySettings(SettingsObject *s);
	void loadSettings(SettingsObject *s);

protected:
	virtual void showEvent(QShowEvent *ev);
	virtual void closeEvent(QCloseEvent *ev);

private
slots:
	void on_ftbLauncherBrowseBtn_clicked();

	void on_ftbBrowseBtn_clicked();

	void on_instDirBrowseBtn_clicked();

	void on_modsDirBrowseBtn_clicked();

	void on_lwjglDirBrowseBtn_clicked();


	void on_jsonEditorBrowseBtn_clicked();

	void on_iconsDirBrowseBtn_clicked();

	void on_maximizedCheckBox_clicked(bool checked);

	void on_buttonBox_accepted();

	void on_buttonBox_rejected();

	void on_javaDetectBtn_clicked();

	void on_javaTestBtn_clicked();

	void on_javaBrowseBtn_clicked();

	void checkFinished(JavaCheckResult result);
private:
	Ui::SettingsDialog *ui;
	std::shared_ptr<JavaChecker> checker;
};
