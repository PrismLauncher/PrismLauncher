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

#include <QListView>
#include <QStyledItemDelegate>
#include <QEvent>
#include <QScrollBar>

class BasePage;
const int pageIconSize = 24;

class PageViewDelegate : public QStyledItemDelegate
{
public:
	PageViewDelegate(QObject *parent) : QStyledItemDelegate(parent)
	{
	}
	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
	{
		QSize size = QStyledItemDelegate::sizeHint(option, index);
		size.setHeight(qMax(size.height(), 32));
		return size;
	}
};

class PageModel : public QAbstractListModel
{
public:
	PageModel(QObject *parent = 0) : QAbstractListModel(parent)
	{
		QPixmap empty(pageIconSize, pageIconSize);
		empty.fill(Qt::transparent);
		m_emptyIcon = QIcon(empty);
	}
	virtual ~PageModel() {}

	int rowCount(const QModelIndex &parent = QModelIndex()) const
	{
		return parent.isValid() ? 0 : m_pages.size();
	}
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const
	{
		switch (role)
		{
		case Qt::DisplayRole:
			return m_pages.at(index.row())->displayName();
		case Qt::DecorationRole:
		{
			QIcon icon = m_pages.at(index.row())->icon();
			if (icon.isNull())
				icon = m_emptyIcon;
			// HACK: fixes icon stretching on windows. TODO: report Qt bug for this
			return QIcon(icon.pixmap(QSize(48,48)));
		}
		}
		return QVariant();
	}

	void setPages(const QList<BasePage *> &pages)
	{
		beginResetModel();
		m_pages = pages;
		endResetModel();
	}
	const QList<BasePage *> &pages() const
	{
		return m_pages;
	}

	BasePage * findPageEntryById(QString id)
	{
		for(auto page: m_pages)
		{
			if (page->id() == id)
				return page;
		}
		return nullptr;
	}

	QList<BasePage *> m_pages;
	QIcon m_emptyIcon;
};

class PageView : public QListView
{
public:
	PageView(QWidget *parent = 0) : QListView(parent)
	{
		setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
		setItemDelegate(new PageViewDelegate(this));
		setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	}

	virtual QSize sizeHint() const
	{
		int width = sizeHintForColumn(0) + frameWidth() * 2 + 5;
		if (verticalScrollBar()->isVisible())
			width += verticalScrollBar()->width();
		return QSize(width, 100);
	}

	virtual bool eventFilter(QObject *obj, QEvent *event)
	{
		if (obj == verticalScrollBar() &&
			(event->type() == QEvent::Show || event->type() == QEvent::Hide))
			updateGeometry();
		return QListView::eventFilter(obj, event);
	}
};
