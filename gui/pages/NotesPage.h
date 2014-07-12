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

#include "logic/BaseInstance.h"
#include "logic/net/NetJob.h"
#include "BasePage.h"

class EnabledItemFilter;
namespace Ui
{
class NotesPage;
}

class NotesPage : public QWidget, public BasePage
{
	Q_OBJECT

public:
	explicit NotesPage(BaseInstance *inst, QWidget *parent = 0);
	virtual ~NotesPage();
	virtual QString displayName() const override
	{
		return tr("Notes");
	}
	virtual QIcon icon() const override
	{
		return QIcon::fromTheme("news");
	}
	virtual QString id() const override
	{
		return "notes";
	}
	virtual bool apply();
	virtual QString helpPage() const override
	{
		return "Notes";
	}

private:
	Ui::NotesPage *ui;
	BaseInstance *m_inst;
};
