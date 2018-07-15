/* Copyright 2013-2018 MultiMC Contributors
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

#include "minecraft/MinecraftInstance.h"
#include "pages/BasePage.h"
#include <MultiMC.h>

class SimpleModList;
namespace Ui
{
class NewModFolderPage;
}

class NewModFolderPage : public QWidget, public BasePage
{
	Q_OBJECT

public:
	explicit NewModFolderPage(BaseInstance *inst, std::shared_ptr<ModsModel> mods, QString id,
						   QString iconName, QString displayName, QString helpPage = "",
						   QWidget *parent = 0);
	virtual ~NewModFolderPage();

	void setFilter(const QString & filter)
	{
		m_fileSelectionFilter = filter;
	}

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
	virtual bool shouldDisplay() const override;

	virtual void openedImpl() override;
	virtual void closedImpl() override;
protected:
	bool eventFilter(QObject *obj, QEvent *ev) override;
	bool modListFilter(QKeyEvent *ev);

protected:
	BaseInstance *m_inst;

protected:
	Ui::NewModFolderPage *ui;
	std::shared_ptr<ModsModel> m_mods;
	QSortFilterProxyModel *m_filterModel;
	QString m_iconName;
	QString m_id;
	QString m_displayName;
	QString m_helpName;
	QString m_fileSelectionFilter;
	QString m_viewFilter;

public
slots:
	void modCurrent(const QModelIndex &current, const QModelIndex &previous);

private
slots:
	void on_filterTextChanged(const QString & newContents);
	void on_addModBtn_clicked();
	void on_rmModBtn_clicked();
	void on_viewModBtn_clicked();
	void on_enableModBtn_clicked();
	void on_disableModBtn_clicked();
	void on_configFolderBtn_clicked();
};

