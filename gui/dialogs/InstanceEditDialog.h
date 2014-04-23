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
#include <logic/net/NetJob.h>

class EnabledItemFilter;
namespace Ui
{
class InstanceEditDialog;
}

class InstanceEditDialog : public QDialog
{
	Q_OBJECT

public:
	explicit InstanceEditDialog(OneSixInstance *inst, QWidget *parent = 0);
	virtual ~InstanceEditDialog();

private
slots:

	// version tab
	void on_forgeBtn_clicked();
	void on_liteloaderBtn_clicked();
	void on_reloadLibrariesBtn_clicked();
	void on_removeLibraryBtn_clicked();
	void on_resetLibraryOrderBtn_clicked();
	void on_settingsBtn_clicked();
	void on_moveLibraryUpBtn_clicked();
	void on_moveLibraryDownBtn_clicked();

	// loader mod tab
	void on_addModBtn_clicked();
	void on_rmModBtn_clicked();
	void on_viewModBtn_clicked();

	// core mod tab
	void on_addCoreBtn_clicked();
	void on_rmCoreBtn_clicked();
	void on_viewCoreBtn_clicked();

	// resource pack tab
	void on_addResPackBtn_clicked();
	void on_rmResPackBtn_clicked();
	void on_viewResPackBtn_clicked();

	
	// Questionable: SettingsDialog doesn't need this for some reason?
	void on_buttonBox_rejected();
	
	void updateVersionControls();
	void disableVersionControls();
	void on_changeMCVersionBtn_clicked();
	
protected:
	bool eventFilter(QObject *obj, QEvent *ev);
	bool jarListFilter(QKeyEvent *ev);
	bool loaderListFilter(QKeyEvent *ev);
	bool coreListFilter(QKeyEvent *ev);
	bool resourcePackListFilter(QKeyEvent *ev);
	/// FIXME: this shouldn't be necessary!
	bool reloadInstanceVersion();

private:
	Ui::InstanceEditDialog *ui;
	std::shared_ptr<VersionFinal> m_version;
	std::shared_ptr<ModList> m_mods;
	std::shared_ptr<ModList> m_coremods;
	std::shared_ptr<ModList> m_jarmods;
	std::shared_ptr<ModList> m_resourcepacks;
	EnabledItemFilter *main_model;
	OneSixInstance *m_inst;
	NetJobPtr forgeJob;

public
slots:
	void loaderCurrent(QModelIndex current, QModelIndex previous);
	void versionCurrent(const QModelIndex &current, const QModelIndex &previous);
	void coreCurrent(QModelIndex current, QModelIndex previous);
};
