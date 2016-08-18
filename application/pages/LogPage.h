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

#include "BaseInstance.h"
#include "launch/LaunchTask.h"
#include "BasePage.h"
#include <MultiMC.h>

namespace Ui
{
class LogPage;
}
class QTextCharFormat;
class LogFormatProxyModel;

class LogPage : public QWidget, public BasePage
{
	Q_OBJECT

public:
	explicit LogPage(InstancePtr instance, QWidget *parent = 0);
	virtual ~LogPage();
	virtual QString displayName() const override
	{
		return tr("Minecraft Log");
	}
	virtual QIcon icon() const override
	{
		return MMC->getThemedIcon("log");
	}
	virtual QString id() const override
	{
		return "console";
	}
	virtual bool apply() override;
	virtual QString helpPage() const override
	{
		return "Minecraft-Logs";
	}
	virtual bool shouldDisplay() const override;
	virtual void setParentContainer(BasePageContainer *) override;

private slots:
	void on_btnPaste_clicked();
	void on_btnCopy_clicked();
	void on_btnClear_clicked();
	void on_btnBottom_clicked();

	void on_trackLogCheckbox_clicked(bool checked);
	void on_wrapCheckbox_clicked(bool checked);

	void on_findButton_clicked();
	void findActivated();
	void findNextActivated();
	void findPreviousActivated();

	void on_InstanceLaunchTask_changed(std::shared_ptr<LaunchTask> proc);

private:
	Ui::LogPage *ui;
	InstancePtr m_instance;
	std::shared_ptr<LaunchTask> m_process;
	int m_last_scroll_value = 0;
	bool m_scroll_active = true;
	int m_saved_offset = 0;
	bool m_write_active = true;
	bool m_stopOnOverflow = true;
	bool m_autoScroll = false;

	BasePageContainer * m_parentContainer;
	LogFormatProxyModel * m_proxy;
	shared_qobject_ptr <LogModel> m_model;
};
