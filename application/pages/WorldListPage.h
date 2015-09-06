/* Copyright 2015 MultiMC Contributors
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

#include "minecraft/OneSixInstance.h"
#include "BasePage.h"
#include <MultiMC.h>
#include <pathutils.h>

class WorldList;
namespace Ui
{
class WorldListPage;
}

class WorldListPage : public QWidget, public BasePage
{
	Q_OBJECT

public:
	explicit WorldListPage(BaseInstance *inst, std::shared_ptr<WorldList> worlds, QString id,
						   QString iconName, QString displayName, QString helpPage = "",
						   QWidget *parent = 0);
	virtual ~WorldListPage();

	virtual QString displayName() const override
	{
		return m_displayName;
	}
	virtual QIcon icon() const override
	{
		return MMC->getThemedIcon(m_iconName);
	}
	virtual QString id() const override
	{
		return m_id;
	}
	virtual QString helpPage() const override
	{
		return m_helpName;
	}
	virtual bool shouldDisplay() const;

	virtual void opened();
	virtual void closed();

protected:
	bool eventFilter(QObject *obj, QEvent *ev);
	bool worldListFilter(QKeyEvent *ev);

protected:
	BaseInstance *m_inst;

private:
	QModelIndex getSelectedWorld();

private:
	Ui::WorldListPage *ui;
	std::shared_ptr<WorldList> m_worlds;
	QString m_iconName;
	QString m_id;
	QString m_displayName;
	QString m_helpName;

private slots:
	void on_copySeedBtn_clicked();
	void on_mcEditBtn_clicked();
	void on_rmWorldBtn_clicked();
	void on_viewFolderBtn_clicked();
	void worldChanged(const QModelIndex &current, const QModelIndex &previous);
};
