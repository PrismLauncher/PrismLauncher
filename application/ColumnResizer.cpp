/*
* Copyright 2011 Aurélien Gâteau <agateau@kde.org>
* License: BSD-3-Clause
*/
#include <ColumnResizer.h>

#include <QDebug>
#include <QEvent>
#include <QFormLayout>
#include <QGridLayout>
#include <QTimer>
#include <QWidget>

class FormLayoutWidgetItem : public QWidgetItem
{
public:
	FormLayoutWidgetItem(QWidget* widget, QFormLayout* formLayout, QFormLayout::ItemRole itemRole)
	: QWidgetItem(widget)
	, m_width(-1)
	, m_formLayout(formLayout)
	, m_itemRole(itemRole)
	{}

	QSize sizeHint() const
	{
		QSize size = QWidgetItem::sizeHint();
		if (m_width != -1) {
			size.setWidth(m_width);
		}
		return size;
	}

	QSize minimumSize() const
	{
		QSize size = QWidgetItem::minimumSize();
		if (m_width != -1) {
			size.setWidth(m_width);
		}
		return size;
	}

	QSize maximumSize() const
	{
		QSize size = QWidgetItem::maximumSize();
		if (m_width != -1) {
			size.setWidth(m_width);
		}
		return size;
	}

	void setWidth(int width)
	{
		if (width != m_width) {
			m_width = width;
			invalidate();
		}
	}

	void setGeometry(const QRect& _rect)
	{
		QRect rect = _rect;
		int width = widget()->sizeHint().width();
		if (m_itemRole == QFormLayout::LabelRole && m_formLayout->labelAlignment() & Qt::AlignRight) {
			rect.setLeft(rect.right() - width);
		}
		QWidgetItem::setGeometry(rect);
	}

	QFormLayout* formLayout() const
	{
		return m_formLayout;
	}

private:
	int m_width;
	QFormLayout* m_formLayout;
	QFormLayout::ItemRole m_itemRole;
};

typedef QPair<QGridLayout*, int> GridColumnInfo;

class ColumnResizerPrivate
{
public:
	ColumnResizerPrivate(ColumnResizer* q_ptr)
	: q(q_ptr)
	, m_updateTimer(new QTimer(q))
	{
		m_updateTimer->setSingleShot(true);
		m_updateTimer->setInterval(0);
		QObject::connect(m_updateTimer, SIGNAL(timeout()), q, SLOT(updateWidth()));
	}

	void scheduleWidthUpdate()
	{
		m_updateTimer->start();
	}

	ColumnResizer* q;
	QTimer* m_updateTimer;
	QList<QWidget*> m_widgets;
	QList<FormLayoutWidgetItem*> m_wrWidgetItemList;
	QList<GridColumnInfo> m_gridColumnInfoList;
};

ColumnResizer::ColumnResizer(QObject* parent)
: QObject(parent)
, d(new ColumnResizerPrivate(this))
{}

ColumnResizer::~ColumnResizer()
{
	delete d;
}

void ColumnResizer::addWidget(QWidget* widget)
{
	d->m_widgets.append(widget);
	widget->installEventFilter(this);
	d->scheduleWidthUpdate();
}

void ColumnResizer::updateWidth()
{
	int width = 0;
	Q_FOREACH(QWidget* widget, d->m_widgets) {
		width = qMax(widget->sizeHint().width(), width);
	}
	Q_FOREACH(FormLayoutWidgetItem* item, d->m_wrWidgetItemList) {
		item->setWidth(width);
		item->formLayout()->update();
	}
	Q_FOREACH(GridColumnInfo info, d->m_gridColumnInfoList) {
		info.first->setColumnMinimumWidth(info.second, width);
	}
}

bool ColumnResizer::eventFilter(QObject*, QEvent* event)
{
	if (event->type() == QEvent::Resize) {
		d->scheduleWidthUpdate();
	}
	return false;
}

void ColumnResizer::addWidgetsFromLayout(QLayout* layout, int column)
{
	Q_ASSERT(column >= 0);
	QGridLayout* gridLayout = qobject_cast<QGridLayout*>(layout);
	QFormLayout* formLayout = qobject_cast<QFormLayout*>(layout);
	if (gridLayout) {
		addWidgetsFromGridLayout(gridLayout, column);
	} else if (formLayout) {
		if (column > QFormLayout::SpanningRole) {
			qCritical() << "column should not be more than" << QFormLayout::SpanningRole << "for QFormLayout";
			return;
		}
		QFormLayout::ItemRole role = static_cast<QFormLayout::ItemRole>(column);
		addWidgetsFromFormLayout(formLayout, role);
	} else {
		qCritical() << "Don't know how to handle layout" << layout;
	}
}

void ColumnResizer::addWidgetsFromGridLayout(QGridLayout* layout, int column)
{
	for (int row = 0; row < layout->rowCount(); ++row) {
		QLayoutItem* item = layout->itemAtPosition(row, column);
		if (!item) {
			continue;
		}
		QWidget* widget = item->widget();
		if (!widget) {
			continue;
		}
		addWidget(widget);
	}
	d->m_gridColumnInfoList << GridColumnInfo(layout, column);
}

void ColumnResizer::addWidgetsFromFormLayout(QFormLayout* layout, QFormLayout::ItemRole role)
{
	for (int row = 0; row < layout->rowCount(); ++row) {
		QLayoutItem* item = layout->itemAt(row, role);
		if (!item) {
			continue;
		}
		QWidget* widget = item->widget();
		if (!widget) {
			continue;
		}
		layout->removeItem(item);
		delete item;
		FormLayoutWidgetItem* newItem = new FormLayoutWidgetItem(widget, layout, role);
		layout->setItem(row, role, newItem);
		addWidget(widget);
		d->m_wrWidgetItemList << newItem;
	}
}

#include <ColumnResizer.moc>