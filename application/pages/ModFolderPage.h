/* Copyright 2013-2015 MultiMC Contributors
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

class ModList;
namespace Ui
{
class ModFolderPage;
}

class ModFolderPage : public QWidget, public BasePage
{
	Q_OBJECT

public:
	explicit ModFolderPage(BaseInstance *inst, std::shared_ptr<ModList> mods, QString id,
						   QString iconName, QString displayName, QString helpPage = "",
						   QWidget *parent = 0);
	virtual ~ModFolderPage();
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
	bool modListFilter(QKeyEvent *ev);

protected:
	BaseInstance *m_inst;

private:
	Ui::ModFolderPage *ui;
	std::shared_ptr<ModList> m_mods;
	QString m_iconName;
	QString m_id;
	QString m_displayName;
	QString m_helpName;

public
slots:
	void modCurrent(const QModelIndex &current, const QModelIndex &previous);

private
slots:
	void on_addModBtn_clicked();
	void on_rmModBtn_clicked();
	void on_viewModBtn_clicked();
};

class CoreModFolderPage : public ModFolderPage
{
public:
	explicit CoreModFolderPage(BaseInstance *inst, std::shared_ptr<ModList> mods, QString id,
							   QString iconName, QString displayName, QString helpPage = "",
							   QWidget *parent = 0);
	virtual ~CoreModFolderPage()
	{
	}
	virtual bool shouldDisplay() const;
};
