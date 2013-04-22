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

#ifndef NEWINSTANCEDIALOG_H
#define NEWINSTANCEDIALOG_H

#include <QDialog>

class InstanceTypeInterface;
class InstVersion;

namespace Ui {
class NewInstanceDialog;
}

class NewInstanceDialog : public QDialog
{
	Q_OBJECT
	
public:
	explicit NewInstanceDialog(QWidget *parent = 0);
	~NewInstanceDialog();
	
	void loadTypeList();
	void updateSelectedType();
	void updateDialogState();
	
	void setSelectedVersion(const InstVersion *version);
	
	void loadVersionList();
	
private slots:
	void on_btnChangeVersion_clicked();
	
	void on_instTypeComboBox_activated(int index);
	
private:
	Ui::NewInstanceDialog *ui;
	
	const InstVersion *m_selectedVersion;
	const InstanceTypeInterface *m_selectedType;
};

#endif // NEWINSTANCEDIALOG_H
