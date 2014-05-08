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
#include "settingsobject.h"
#include "logic/java/JavaChecker.h"

namespace Ui
{
class InstanceSettings;
}

class InstanceSettings : public QDialog
{
	Q_OBJECT

public:
	explicit InstanceSettings(SettingsObject *s, QWidget *parent = 0);
	~InstanceSettings();

	void updateCheckboxStuff();

	void applySettings();
	void loadSettings();

protected:
	virtual void showEvent(QShowEvent *);
	virtual void closeEvent(QCloseEvent *);
private
slots:
	void on_customCommandsGroupBox_toggled(bool arg1);
	void on_buttonBox_accepted();
	void on_buttonBox_rejected();

	void on_javaDetectBtn_clicked();

	void on_javaTestBtn_clicked();

	void on_javaBrowseBtn_clicked();

	void checkFinished(JavaCheckResult result);
private:
	Ui::InstanceSettings *ui;
	SettingsObject *m_obj;
	std::shared_ptr<JavaChecker> checker;
};
