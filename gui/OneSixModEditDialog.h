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

class EnabledItemFilter;
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
	void on_forgeBtn_clicked();
	void on_customizeBtn_clicked();
	void on_revertBtn_clicked();
    void updateVersionControls();
	void disableVersionControls();

	void on_loaderModTreeView_pressed(const QModelIndex &index);

protected:
	bool eventFilter(QObject *obj, QEvent *ev);
	bool loaderListFilter( QKeyEvent* ev );
	bool resourcePackListFilter( QKeyEvent* ev );
private:
	Ui::OneSixModEditDialog *ui;
	std::shared_ptr<OneSixVersion> m_version;
	std::shared_ptr<ModList> m_mods;
	std::shared_ptr<ModList> m_resourcepacks;
	EnabledItemFilter * main_model;
	OneSixInstance * m_inst;
};
