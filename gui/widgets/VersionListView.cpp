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

#include <QHeaderView>
#include <QApplication>
#include <QMouseEvent>
#include <QDrag>
#include <QPainter>
#include "VersionListView.h"
#include "Common.h"

VersionListView::VersionListView(QWidget *parent)
	:QTreeView ( parent )
{
	m_emptyString = tr("No versions are currently available.");
}

void VersionListView::rowsInserted(const QModelIndex &parent, int start, int end)
{
	if(!m_itemCount)
		viewport()->update();
	m_itemCount += end-start+1;
	QTreeView::rowsInserted(parent, start, end);
}


void VersionListView::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
	m_itemCount -= end-start+1;
	if(!m_itemCount)
		viewport()->update();
	QTreeView::rowsInserted(parent, start, end);
}

void VersionListView::setModel(QAbstractItemModel *model)
{
	m_itemCount = model->rowCount();
	if(!m_itemCount)
		viewport()->update();
	QTreeView::setModel(model);
}

void VersionListView::reset()
{
	if(model())
	{
		m_itemCount = model()->rowCount();
	}
	viewport()->update();
	QTreeView::reset();
}

void VersionListView::setEmptyString(QString emptyString)
{
	m_emptyString = emptyString;
	if(!m_itemCount)
	{
		viewport()->update();
	}
}

void VersionListView::paintEvent(QPaintEvent *event)
{
	if(m_itemCount)
	{
		QTreeView::paintEvent(event);
	}
	else
	{
		paintInfoLabel(event);
	}
}

void VersionListView::paintInfoLabel(QPaintEvent *event)
{
    //calculate the rect for the overlay
    QPainter painter(viewport());
    painter.setRenderHint(QPainter::Antialiasing, true);
	QFont font("sans", 20);
    font.setBold(true);
	
	QRect bounds = viewport()->geometry();
	bounds.moveTop(0);
	QTextLayout layout(m_emptyString, font);
	qreal height = 0.0;
	qreal widthUsed = 0.0;
	QStringList lines = viewItemTextLayout(layout, bounds.width() - 20, height, widthUsed);
	QRect rect (0,0, widthUsed, height);
	rect.setWidth(rect.width()+20);
	rect.setHeight(rect.height()+20);
    rect.moveCenter(bounds.center());
    //check if we are allowed to draw in our area
    if (!event->rect().intersects(rect)) {
        return;
    }
    //draw the letter of the topmost item semitransparent in the middle
    QColor background = QApplication::palette().color(QPalette::Foreground);
    QColor foreground = QApplication::palette().color(QPalette::Base);
	/*
    background.setAlpha(128 - scrollFade);
    foreground.setAlpha(128 - scrollFade);
    */
    painter.setBrush(QBrush(background));
    painter.setPen(foreground);
    painter.drawRoundedRect(rect, 5.0, 5.0);
    foreground.setAlpha(190);
    painter.setPen(foreground);
    painter.setFont(font);
    painter.drawText(rect, Qt::AlignCenter, lines.join("\n"));
	
}

/*
void ModListView::setModel ( QAbstractItemModel* model )
{
	QTreeView::setModel ( model );
	auto head = header();
	head->setStretchLastSection(false);
	// HACK: this is true for the checkbox column of mod lists
	auto string = model->headerData(0,head->orientation()).toString();
	if(!string.size())
	{
		head->setSectionResizeMode(0, QHeaderView::ResizeToContents);
		head->setSectionResizeMode(1, QHeaderView::Stretch);
		for(int i = 2; i < head->count(); i++)
			head->setSectionResizeMode(i, QHeaderView::ResizeToContents);
	}
	else
	{
		head->setSectionResizeMode(0, QHeaderView::Stretch);
		for(int i = 1; i < head->count(); i++)
			head->setSectionResizeMode(i, QHeaderView::ResizeToContents);
	}
}
*/
