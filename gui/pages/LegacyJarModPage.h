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
#include <logic/net/NetJob.h>
#include "BasePage.h"

class ModList;
class LegacyInstance;
namespace Ui
{
class LegacyJarModPage;
}

class LegacyJarModPage : public QWidget, public BasePage
{
	Q_OBJECT

public:
	explicit LegacyJarModPage(LegacyInstance *inst, QWidget *parent = 0);
	virtual ~LegacyJarModPage();

	virtual QString displayName() const;
	virtual QIcon icon() const;
	virtual QString id() const;
	virtual QString helpPage() const override { return "Legacy-jar-mods"; }
	virtual bool shouldDisplay() const;

private
slots:

	void on_addJarBtn_clicked();
	void on_rmJarBtn_clicked();
	void on_addForgeBtn_clicked();
	void on_moveJarUpBtn_clicked();
	void on_moveJarDownBtn_clicked();
	void on_viewJarBtn_clicked();

	void jarCurrent(QModelIndex current, QModelIndex previous);

protected:
	virtual bool eventFilter(QObject *obj, QEvent *ev) override;

private:
	Ui::LegacyJarModPage *ui;
	std::shared_ptr<ModList> m_jarmods;
	LegacyInstance *m_inst;
	NetJobPtr forgeJob;
};
