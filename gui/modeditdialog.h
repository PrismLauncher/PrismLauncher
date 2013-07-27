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

#ifndef MODEDITDIALOG_H
#define MODEDITDIALOG_H

#include <QDialog>

#include "instance.h"

namespace Ui {
	class ModEditDialog;
}

class ModEditDialog : public QDialog
{
	Q_OBJECT
	
public:
	explicit ModEditDialog(QWidget *parent = 0, Instance* m_inst = 0);
	~ModEditDialog();
	
private slots:
	/* Mapped for implementation
	void on_addModBtn_clicked();
	void on_rmModBtn_clicked();
	void on_viewModBtn_clicked();
	 
	void on_addResPackBtn_clicked();
	void on_rmResPackBtn_clicked();
	void on_viewResPackBtn_clicked();
	*/
	// Questionable: SettingsDialog doesn't need this for some reason?
	void on_buttonBox_rejected();
	
private:
	Ui::ModEditDialog *ui;
};

#endif // MODEDITDIALOG_H