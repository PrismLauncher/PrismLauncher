/* Copyright 2013-2021 MultiMC Contributors
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
#include <QHeaderView>
#include <QTreeView>

class ModListView: public QTreeView
{
    Q_OBJECT
public:
    explicit ModListView ( QWidget* parent = 0 );
    virtual void setModel ( QAbstractItemModel* model );
    virtual void setResizeModes (const QList<QHeaderView::ResizeMode>& modes);
};
