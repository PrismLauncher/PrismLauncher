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
#include "logic/LegacyInstance.h"

namespace Ui {
class LegacyModEditDialog;
}

class LegacyModEditDialog : public QDialog
{
	Q_OBJECT
	
public:
	explicit LegacyModEditDialog(LegacyInstance* inst, QWidget *parent = 0);
	~LegacyModEditDialog();
	
private slots:
	/* Mapped for implementation
	void on_addJarBtn_clicked();
	void on_rmJarBtn_clicked();     
	void on_addForgeBtn_clicked();
	void on_moveJarUpBtn_clicked();
	void on_moveJarDownBtn_clicked();
	
	void on_addCoreBtn_clicked();
	void on_rmCoreBtn_clicked();
	void on_viewCoreBtn_clicked();
	
	void on_addModBtn_clicked();
	void on_rmModBtn_clicked();
	void on_viewModBtn_clicked();
	
	void on_addTexPackBtn_clicked();
	void on_rmTexPackBtn_clicked();
	void on_viewTexPackBtn_clicked();
	*/
	// Questionable: SettingsDialog doesn't need this for some reason?
	void on_buttonBox_rejected();

private:
	Ui::LegacyModEditDialog *ui;
	LegacyInstance * m_inst;
};
