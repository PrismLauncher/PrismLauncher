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

#include "gui/pages/BasePage.h"

namespace Ui {
class ExternalToolsPage;
}

class ExternalToolsPage : public QWidget, public BasePage
{
	Q_OBJECT

public:
	explicit ExternalToolsPage(QWidget *parent = 0);
	~ExternalToolsPage();

	QString displayName() const override
	{
		return tr("External Tools");
	}
	QIcon icon() const override
	{
		return QIcon::fromTheme("externaltools");
	}
	QString id() const override
	{
		return "external-tools";
	}
	QString helpPage() const override
	{
		return "External-tools";
	}
	virtual bool apply();

private:
	void loadSettings();
	void applySettings();

private:
	Ui::ExternalToolsPage *ui;

private
slots:
	void on_jprofilerPathBtn_clicked();
	void on_jprofilerCheckBtn_clicked();
	void on_jvisualvmPathBtn_clicked();
	void on_jvisualvmCheckBtn_clicked();
	void on_mceditPathBtn_clicked();
	void on_mceditCheckBtn_clicked();
	void on_jsonEditorBrowseBtn_clicked();
};
