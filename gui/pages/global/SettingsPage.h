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

#include <memory>
#include <QDialog>

#include "logic/java/JavaChecker.h"
#include "BaseSettingsPage.h"

class SettingsObject;

namespace Ui
{
class SettingsPage;
}

class SettingsPage : public QWidget, public BaseSettingsPage
{
	Q_OBJECT

public:
	explicit SettingsPage(QWidget *parent = 0);
	~SettingsPage();

	QString displayName() const override
	{
		return tr("Settings");
	}
	QIcon icon() const override
	{
		return QIcon::fromTheme("settings");
	}
	QString id() const override
	{
		return "global-settings";
	}
	QString helpPage() const override
	{
		return "Global-settings";
	}

	void updateCheckboxStuff();


protected:
	void applySettings(SettingsObject *s) override;
	void loadSettings(SettingsObject *s) override;
	virtual void closeEvent(QCloseEvent *ev);

private
slots:
	void on_ftbLauncherBrowseBtn_clicked();
	void on_ftbBrowseBtn_clicked();

	void on_instDirBrowseBtn_clicked();
	void on_modsDirBrowseBtn_clicked();
	void on_lwjglDirBrowseBtn_clicked();
	void on_iconsDirBrowseBtn_clicked();

	void on_jsonEditorBrowseBtn_clicked();

	void on_maximizedCheckBox_clicked(bool checked);

	void on_javaDetectBtn_clicked();
	void on_javaTestBtn_clicked();
	void on_javaBrowseBtn_clicked();

	void checkFinished(JavaCheckResult result);

	/*!
	 * Updates the list of update channels in the combo box.
	 */
	void refreshUpdateChannelList();

	/*!
	 * Updates the channel description label.
	 */
	void refreshUpdateChannelDesc();

    void updateChannelSelectionChanged(int index);
	void proxyChanged(int);

private:
	Ui::SettingsPage *ui;
	std::shared_ptr<JavaChecker> checker;

	/*!
	 * Stores the currently selected update channel.
	 */
	QString m_currentUpdateChannel;
};
