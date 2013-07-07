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

#ifndef MODEDITWINDOW_H
#define MODEDITWINDOW_H

#include <QDialog>

#include "instance.h"

namespace Ui {
class ModEditWindow;
}

class ModEditWindow : public QDialog
{
	Q_OBJECT
	
public:
	explicit ModEditWindow(QWidget *parent = 0, Instance* m_inst = 0);
	~ModEditWindow();
	
private slots:
	/* Mapped for implementation
	void on_addTPackButton_clicked();
	void on_delTPackButton_clicked();
	void on_viewTPackButton_clicked();
	
	void on_addMlModButton_clicked();
	void on_delMlModButton_clicked();
	void on_viewMlModbutton_clicked();
	
	void on_addCoreModButton_clicked();
	void on_delCoreModButton_clicked();
	void on_viewCoreModButton_clicked();
	
	void on_addJarModButton_clicked();
	void on_delJarModButton_clicked();
	void on_mcforgeButton_clicked();
	void on_jarModMoveUpButton_clicked();
	void on_jarModMoveDownButton_clicked();
	*/
	// Questionable: SettingsDialog doesn't need this for some reason?
	void on_buttonBox_rejected();
	
private:
	Ui::ModEditWindow *ui;
};

#endif // MODEDITWINDOW_H
