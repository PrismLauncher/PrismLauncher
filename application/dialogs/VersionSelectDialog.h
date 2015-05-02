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

#include <QDialog>
#include <QSortFilterProxyModel>

#include "BaseVersionList.h"

namespace Ui
{
class VersionSelectDialog;
}

class VersionProxyModel;

class VersionSelectDialog : public QDialog
{
	Q_OBJECT

public:
	explicit VersionSelectDialog(BaseVersionList *vlist, QString title, QWidget *parent = 0,
								 bool cancelable = true);
	~VersionSelectDialog();

	virtual int exec();

	//! Starts a task that loads the list.
	void loadList();

	BaseVersionPtr selectedVersion() const;

	void setFuzzyFilter(BaseVersionList::ModelRoles role, QString filter);
	void setExactFilter(BaseVersionList::ModelRoles role, QString filter);
	void setEmptyString(QString emptyString);
	void setEmptyErrorString(QString emptyErrorString);
	void setResizeOn(int column);
	void setUseLatest(const bool useLatest);

protected:
    virtual void closeEvent ( QCloseEvent* );

private
slots:
	void on_refreshButton_clicked();

	void onTaskFinished();
	void changeProgress(qint64 current, qint64 total);

private:
	void preselect();
	void selectRecommended();

private:
	Ui::VersionSelectDialog *ui = nullptr;

	BaseVersionList *m_vlist = nullptr;

	VersionProxyModel *m_proxyModel = nullptr;

	int resizeOnColumn = 0;

	Task * loadTask = nullptr;

	bool preselectedAlready = false;
};
