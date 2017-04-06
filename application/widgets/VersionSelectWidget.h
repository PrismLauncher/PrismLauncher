/* Copyright 2013-2017 MultiMC Contributors
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
#include <QSortFilterProxyModel>
#include "BaseVersionList.h"

class VersionProxyModel;
class VersionListView;
class QVBoxLayout;
class QProgressBar;

class VersionSelectWidget: public QWidget
{
	Q_OBJECT
public:
	explicit VersionSelectWidget(BaseVersionList *vlist, QWidget *parent = 0);
	~VersionSelectWidget();

	//! loads the list if needed.
	void initialize();

	//! Starts a task that loads the list.
	void loadList();

	bool hasVersions() const;
	BaseVersionPtr selectedVersion() const;
	void selectRecommended();

	void setFuzzyFilter(BaseVersionList::ModelRoles role, QString filter);
	void setExactFilter(BaseVersionList::ModelRoles role, QString filter);
	void setEmptyString(QString emptyString);
	void setEmptyErrorString(QString emptyErrorString);
	void setResizeOn(int column);
	void setUseLatest(const bool useLatest);

signals:
	void selectedVersionChanged(BaseVersionPtr version);

protected:
	virtual void closeEvent ( QCloseEvent* );

private slots:
	void onTaskSucceeded();
	void onTaskFailed(const QString &reason);
	void changeProgress(qint64 current, qint64 total);
	void currentRowChanged(const QModelIndex &current, const QModelIndex &);

private:
	void preselect();

private:
	BaseVersionList *m_vlist = nullptr;
	VersionProxyModel *m_proxyModel = nullptr;
	int resizeOnColumn = 0;
	Task * loadTask;
	bool preselectedAlready = false;

private:
	QVBoxLayout *verticalLayout = nullptr;
	VersionListView *listView = nullptr;
	QProgressBar *sneakyProgressBar = nullptr;
};
