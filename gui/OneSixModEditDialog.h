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

#include <logic/OneSixInstance.h>

namespace Ui {
	class OneSixModEditDialog;
}

class OneSixModEditDialog : public QDialog
{
	Q_OBJECT
	
public:
	explicit OneSixModEditDialog(OneSixInstance* inst, QWidget *parent = 0);
	virtual ~OneSixModEditDialog();
	
private slots:
	void on_addModBtn_clicked();
	void on_rmModBtn_clicked();
	void on_viewModBtn_clicked();
	 
	void on_addResPackBtn_clicked();
	void on_rmResPackBtn_clicked();
	void on_viewResPackBtn_clicked();
	// Questionable: SettingsDialog doesn't need this for some reason?
	void on_buttonBox_rejected();
protected:
	bool eventFilter(QObject *obj, QEvent *ev);
	bool loaderListFilter( QKeyEvent* ev );
	bool resourcePackListFilter( QKeyEvent* ev );
private:
	Ui::OneSixModEditDialog *ui;
	QSharedPointer<OneSixVersion> m_version;
	QSharedPointer<ModList> m_mods;
	QSharedPointer<ModList> m_resourcepacks;
	OneSixInstance * m_inst;
};
