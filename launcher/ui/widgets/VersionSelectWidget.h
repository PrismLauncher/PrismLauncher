// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#pragma once

#include <QWidget>
#include <QSortFilterProxyModel>
#include <QLineEdit>
#include "BaseVersionList.h"
#include "VersionListView.h"

class VersionProxyModel;
class VersionListView;
class QVBoxLayout;
class QProgressBar;
class Filter;

class VersionSelectWidget: public QWidget
{
    Q_OBJECT
public:
    explicit VersionSelectWidget(QWidget *parent);
    explicit VersionSelectWidget(bool focusSearch = false, QWidget *parent = 0);
    ~VersionSelectWidget();

    //! loads the list if needed.
    void initialize(BaseVersionList *vlist);

    //! Starts a task that loads the list.
    void loadList();

    bool hasVersions() const;
    BaseVersion::Ptr selectedVersion() const;
    void selectRecommended();
    void selectCurrent();

    void setCurrentVersion(const QString & version);
    void setFuzzyFilter(BaseVersionList::ModelRoles role, QString filter);
    void setExactFilter(BaseVersionList::ModelRoles role, QString filter);
    void setFilter(BaseVersionList::ModelRoles role, Filter *filter);
    void setEmptyString(QString emptyString);
    void setEmptyErrorString(QString emptyErrorString);
    void setEmptyMode(VersionListView::EmptyMode mode);
    void setResizeOn(int column);
    bool eventFilter(QObject* watched, QEvent* event) override;

signals:
    void selectedVersionChanged(BaseVersion::Ptr version);

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
    QString m_currentVersion;
    BaseVersionList *m_vlist = nullptr;
    VersionProxyModel *m_proxyModel = nullptr;
    int resizeOnColumn = 0;
    Task * loadTask;
    bool preselectedAlready = false;
    bool focusSearch;

    QVBoxLayout *verticalLayout = nullptr;
    VersionListView *listView = nullptr;
    QLineEdit *search;
    QProgressBar *sneakyProgressBar = nullptr;
};
