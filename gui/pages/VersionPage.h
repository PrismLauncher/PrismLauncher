/* Copyright 2014 MultiMC Contributors
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
#include <QWidget>

#include <logic/OneSixInstance.h>
#include <logic/net/NetJob.h>
#include "BasePage.h"

class EnabledItemFilter;
namespace Ui
{
class VersionPage;
}

class VersionPage : public QWidget, public BasePage
{
	Q_OBJECT

public:
	explicit VersionPage(OneSixInstance *inst, QWidget *parent = 0);
	virtual ~VersionPage();
	virtual QString displayName() override;
	virtual QIcon icon() override;
	virtual QString id() override;
	virtual QString helpPage() override { return "Instance-version"; };
private
slots:

	// version tab
	void on_forgeBtn_clicked();
	void on_liteloaderBtn_clicked();
	void on_reloadLibrariesBtn_clicked();
	void on_removeLibraryBtn_clicked();
	void on_resetLibraryOrderBtn_clicked();
	void on_moveLibraryUpBtn_clicked();
	void on_moveLibraryDownBtn_clicked();
	void on_jarmodBtn_clicked();

	void updateVersionControls();
	void disableVersionControls();
	void on_changeMCVersionBtn_clicked();

protected:
	/// FIXME: this shouldn't be necessary!
	bool reloadInstanceVersion();

private:
	Ui::VersionPage *ui;
	std::shared_ptr<InstanceVersion> m_version;
	EnabledItemFilter *main_model;
	OneSixInstance *m_inst;
	NetJobPtr forgeJob;

public
slots:
	void versionCurrent(const QModelIndex &current, const QModelIndex &previous);
};
