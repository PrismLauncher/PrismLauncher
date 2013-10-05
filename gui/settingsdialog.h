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

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

class SettingsObject;

namespace Ui {
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
	void loadSettings(SettingsObject* s);

protected:
	virtual void showEvent ( QShowEvent* );
	
private slots:
	void on_instDirBrowseBtn_clicked();
	
	void on_modsDirBrowseBtn_clicked();
	
	void on_lwjglDirBrowseBtn_clicked();
	
	void on_compatModeCheckBox_clicked(bool checked);
	
	void on_maximizedCheckBox_clicked(bool checked);
	
	void on_buttonBox_accepted();
	
	void on_pushButton_clicked();

	void on_btnBrowse_clicked();

private:
	Ui::SettingsDialog *ui;
};

#endif // SETTINGSDIALOG_H
