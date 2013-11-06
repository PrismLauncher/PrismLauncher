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

#include "ModListView.h"
#include <QHeaderView>
#include <QMouseEvent>
#include <QPainter>
#include <QDrag>
#include <QRect>

ModListView::ModListView ( QWidget* parent )
	:QTreeView ( parent )
{
	setAllColumnsShowFocus ( true );
	setExpandsOnDoubleClick ( false );
	setRootIsDecorated ( false );
	setSortingEnabled ( false );
	setAlternatingRowColors ( true );
	setSelectionMode ( QAbstractItemView::ContiguousSelection );
	setHeaderHidden ( false );
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setVerticalScrollBarPolicy ( Qt::ScrollBarAlwaysOn );
	setHorizontalScrollBarPolicy ( Qt::ScrollBarAsNeeded );
	setDropIndicatorShown(true);
	setDragEnabled(true);
	setDragDropMode(QAbstractItemView::DropOnly);
	viewport()->setAcceptDrops(true);
}

void ModListView::setModel ( QAbstractItemModel* model )
{
	QTreeView::setModel ( model );
	auto head = header();
	head->setStretchLastSection(false);
	head->setSectionResizeMode(0, QHeaderView::Stretch);
	for(int i = 1; i < head->count(); i++)
		head->setSectionResizeMode(i, QHeaderView::ResizeToContents);
	dropIndicatorPosition();
}
