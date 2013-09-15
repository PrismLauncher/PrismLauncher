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

#ifndef VERSIONSELECTDIALOG_H
#define VERSIONSELECTDIALOG_H

#include <QDialog>

#include <QSortFilterProxyModel>
#include "logic/BaseVersion.h"

class BaseVersionList;

namespace Ui
{
class VersionSelectDialog;
}

class VersionSelectDialog : public QDialog
{
	Q_OBJECT
	
public:
	explicit VersionSelectDialog(BaseVersionList *vlist, QWidget *parent = 0);
	~VersionSelectDialog();
	
	virtual int exec();
	
	//! Starts a task that loads the list.
	void loadList();
	
	BaseVersionPtr selectedVersion() const;
	
private slots:
	void on_refreshButton_clicked();
	
	void updateFilterState();
	
private:
	Ui::VersionSelectDialog *ui;
	
	BaseVersionList *m_vlist;
	
	QSortFilterProxyModel *m_proxyModel;
};

#endif // VERSIONSELECTDIALOG_H
