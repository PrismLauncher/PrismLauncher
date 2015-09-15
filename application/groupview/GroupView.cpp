#include "GroupView.h"

#include <QPainter>
#include <QApplication>
#include <QtMath>
#include <QMouseEvent>
#include <QListView>
#include <QPersistentModelIndex>
#include <QDrag>
#include <QMimeData>
#include <QCache>
#include <QScrollBar>

#include "VisualGroup.h"
#include <QDebug>

template <typename T> bool listsIntersect(const QList<T> &l1, const QList<T> t2)
{
	for (auto &item : l1)
	{
		if (t2.contains(item))
		{
			return true;
		}
	}
	return false;
}

GroupView::GroupView(QWidget *parent)
	: QAbstractItemView(parent)
{
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	setAcceptDrops(true);
	setAutoScroll(true);
}

GroupView::~GroupView()
{
	qDeleteAll(m_groups);
	m_groups.clear();
}

void GroupView::setModel(QAbstractItemModel *model)
{
	QAbstractItemView::setModel(model);
	connect(model, &QAbstractItemModel::modelReset, this, &GroupView::modelReset);
}

void GroupView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight,
							const QVector<int> &roles)
{
	scheduleDelayedItemsLayout();
}
void GroupView::rowsInserted(const QModelIndex &parent, int start, int end)
{
	scheduleDelayedItemsLayout();
}

void GroupView::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
	scheduleDelayedItemsLayout();
}

class LocaleString : public QString
{
public:
	LocaleString(const char *s) : QString(s)
	{
	}
	LocaleString(const QString &s) : QString(s)
	{
	}
};

inline bool operator<(const LocaleString &lhs, const LocaleString &rhs)
{
	return (QString::localeAwareCompare(lhs, rhs) < 0);
}

void GroupView::updateGeometries()
{
	geometryCache.clear();
	int previousScroll = verticalScrollBar()->value();

	QMap<LocaleString, VisualGroup *> cats;

	for (int i = 0; i < model()->rowCount(); ++i)
	{
		const QString groupName =
			model()->index(i, 0).data(GroupViewRoles::GroupRole).toString();
		if (!cats.contains(groupName))
		{
			VisualGroup *old = this->category(groupName);
			if (old)
			{
				cats.insert(groupName, new VisualGroup(old));
			}
			else
			{
				cats.insert(groupName, new VisualGroup(groupName, this));
			}
		}
	}

	/*if (m_editedCategory)
	{
		m_editedCategory = cats[m_editedCategory->text];
	}*/

	qDeleteAll(m_groups);
	m_groups = cats.values();

	for (auto cat : m_groups)
	{
		cat->update();
	}

	if (m_groups.isEmpty())
	{
		verticalScrollBar()->setRange(0, 0);
	}
	else
	{
		int totalHeight = 0;
		// top margin
		totalHeight += m_categoryMargin;
		int itemScroll = 0;
		for (auto category : m_groups)
		{
			category->m_verticalPosition = totalHeight;
			totalHeight += category->totalHeight() + m_categoryMargin;
			if(!itemScroll && category->totalHeight() != 0)
			{
				itemScroll = category->contentHeight() / category->numRows();
			}
		}
		// do not divide by zero
		if(itemScroll == 0)
			itemScroll = 64;

		totalHeight += m_bottomMargin;
		verticalScrollBar()->setSingleStep ( itemScroll );
		const int rowsPerPage = qMax ( viewport()->height() / itemScroll, 1 );
		verticalScrollBar()->setPageStep ( rowsPerPage * itemScroll );

		verticalScrollBar()->setRange(0, totalHeight - height());
	}

	verticalScrollBar()->setValue(qMin(previousScroll, verticalScrollBar()->maximum()));

	viewport()->update();
}

void GroupView::modelReset()
{
	scheduleDelayedItemsLayout();
	executeDelayedItemsLayout();
}

bool GroupView::isIndexHidden(const QModelIndex &index) const
{
	VisualGroup *cat = category(index);
	if (cat)
	{
		return cat->collapsed;
	}
	else
	{
		return false;
	}
}

VisualGroup *GroupView::category(const QModelIndex &index) const
{
	return category(index.data(GroupViewRoles::GroupRole).toString());
}

VisualGroup *GroupView::category(const QString &cat) const
{
	for (auto group : m_groups)
	{
		if (group->text == cat)
		{
			return group;
		}
	}
	return nullptr;
}

VisualGroup *GroupView::categoryAt(const QPoint &pos) const
{
	for (auto group : m_groups)
	{
		if(group->hitScan(pos) & VisualGroup::CheckboxHit)
		{
			return group;
		}
	}
	return nullptr;
}

int GroupView::calculateItemsPerRow() const
{
	return qFloor((qreal)(contentWidth()) / (qreal)(itemWidth() + m_spacing));
}

int GroupView::contentWidth() const
{
	return width() - m_leftMargin - m_rightMargin;
}

int GroupView::itemWidth() const
{
	return m_itemWidth;
}

void GroupView::mousePressEvent(QMouseEvent *event)
{
	// endCategoryEditor();

	QPoint visualPos = event->pos();
	QPoint geometryPos = event->pos() + offset();

	QPersistentModelIndex index = indexAt(visualPos);

	m_pressedIndex = index;
	m_pressedAlreadySelected = selectionModel()->isSelected(m_pressedIndex);
	m_pressedPosition = geometryPos;

	m_pressedCategory = categoryAt(geometryPos);
	if (m_pressedCategory)
	{
		setState(m_pressedCategory->collapsed ? ExpandingState : CollapsingState);
		event->accept();
		return;
	}

	if (index.isValid() && (index.flags() & Qt::ItemIsEnabled))
	{
		if(index != currentIndex())
		{
			// FIXME: better!
			m_currentCursorColumn = -1;
		}
		// we disable scrollTo for mouse press so the item doesn't change position
		// when the user is interacting with it (ie. clicking on it)
		bool autoScroll = hasAutoScroll();
		setAutoScroll(false);
		selectionModel()->setCurrentIndex(index, QItemSelectionModel::NoUpdate);

		setAutoScroll(autoScroll);
		QRect rect(visualPos, visualPos);
		setSelection(rect, QItemSelectionModel::ClearAndSelect);

		// signal handlers may change the model
		emit pressed(index);
	}
	else
	{
		// Forces a finalize() even if mouse is pressed, but not on a item
		selectionModel()->select(QModelIndex(), QItemSelectionModel::Select);
	}
}

void GroupView::mouseMoveEvent(QMouseEvent *event)
{
	QPoint topLeft;
	QPoint visualPos = event->pos();
	QPoint geometryPos = event->pos() + offset();

	if (state() == ExpandingState || state() == CollapsingState)
	{
		return;
	}

	if (state() == DraggingState)
	{
		topLeft = m_pressedPosition - offset();
		if ((topLeft - event->pos()).manhattanLength() > QApplication::startDragDistance())
		{
			m_pressedIndex = QModelIndex();
			startDrag(model()->supportedDragActions());
			setState(NoState);
			stopAutoScroll();
		}
		return;
	}

	if (selectionMode() != SingleSelection)
	{
		topLeft = m_pressedPosition - offset();
	}
	else
	{
		topLeft = geometryPos;
	}

	if (m_pressedIndex.isValid() && (state() != DragSelectingState) &&
		(event->buttons() != Qt::NoButton) && !selectedIndexes().isEmpty())
	{
		setState(DraggingState);
		return;
	}

	if ((event->buttons() & Qt::LeftButton) && selectionModel())
	{
		setState(DragSelectingState);

		setSelection(QRect(visualPos, visualPos), QItemSelectionModel::ClearAndSelect);
		QModelIndex index = indexAt(visualPos);

		// set at the end because it might scroll the view
		if (index.isValid() && (index != selectionModel()->currentIndex()) &&
			(index.flags() & Qt::ItemIsEnabled))
		{
			selectionModel()->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
		}
	}
}

void GroupView::mouseReleaseEvent(QMouseEvent *event)
{
	QPoint visualPos = event->pos();
	QPoint geometryPos = event->pos() + offset();
	QPersistentModelIndex index = indexAt(visualPos);

	bool click = (index == m_pressedIndex && index.isValid()) ||
				 (m_pressedCategory && m_pressedCategory == categoryAt(geometryPos));

	if (click && m_pressedCategory)
	{
		if (state() == ExpandingState)
		{
			m_pressedCategory->collapsed = false;
			updateGeometries();
			viewport()->update();
			event->accept();
			return;
		}
		else if (state() == CollapsingState)
		{
			m_pressedCategory->collapsed = true;
			updateGeometries();
			viewport()->update();
			event->accept();
			return;
		}
	}

	m_ctrlDragSelectionFlag = QItemSelectionModel::NoUpdate;

	setState(NoState);

	if (click)
	{
		if (event->button() == Qt::LeftButton)
		{
			emit clicked(index);
		}
		QStyleOptionViewItem option = viewOptions();
		if (m_pressedAlreadySelected)
		{
			option.state |= QStyle::State_Selected;
		}
		if ((model()->flags(index) & Qt::ItemIsEnabled) &&
			style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick, &option, this))
		{
			emit activated(index);
		}
	}
}

void GroupView::mouseDoubleClickEvent(QMouseEvent *event)
{
	QModelIndex index = indexAt(event->pos());
	if (!index.isValid() || !(index.flags() & Qt::ItemIsEnabled) || (m_pressedIndex != index))
	{
		QMouseEvent me(QEvent::MouseButtonPress, event->localPos(), event->windowPos(),
					   event->screenPos(), event->button(), event->buttons(),
					   event->modifiers());
		mousePressEvent(&me);
		return;
	}
	// signal handlers may change the model
	QPersistentModelIndex persistent = index;
	emit doubleClicked(persistent);
}

void GroupView::paintEvent(QPaintEvent *event)
{
	executeDelayedItemsLayout();

	QPainter painter(this->viewport());

	QStyleOptionViewItemV4 option(viewOptions());
	option.widget = this;

	int wpWidth = viewport()->width();
	option.rect.setWidth(wpWidth);
	for (int i = 0; i < m_groups.size(); ++i)
	{
		VisualGroup *category = m_groups.at(i);
		int y = category->verticalPosition();
		y -= verticalOffset();
		QRect backup = option.rect;
		int height = category->totalHeight();
		option.rect.setTop(y);
		option.rect.setHeight(height);
		option.rect.setLeft(m_leftMargin);
		option.rect.setRight(wpWidth - m_rightMargin);
		category->drawHeader(&painter, option);
		y += category->totalHeight() + m_categoryMargin;
		option.rect = backup;
	}

	for (int i = 0; i < model()->rowCount(); ++i)
	{
		const QModelIndex index = model()->index(i, 0);
		if (isIndexHidden(index))
		{
			continue;
		}
		Qt::ItemFlags flags = index.flags();
		option.rect = visualRect(index);
		option.features |=
			QStyleOptionViewItemV2::WrapText; // FIXME: what is the meaning of this anyway?
		if (flags & Qt::ItemIsSelectable && selectionModel()->isSelected(index))
		{
			option.state |= selectionModel()->isSelected(index) ? QStyle::State_Selected
																: QStyle::State_None;
		}
		else
		{
			option.state &= ~QStyle::State_Selected;
		}
		option.state |= (index == currentIndex()) ? QStyle::State_HasFocus : QStyle::State_None;
		if (!(flags & Qt::ItemIsEnabled))
		{
			option.state &= ~QStyle::State_Enabled;
		}
		itemDelegate()->paint(&painter, option, index);
	}

	/*
	 * Drop indicators for manual reordering...
	 */
#if 0
	if (!m_lastDragPosition.isNull())
	{
		QPair<Group *, int> pair = rowDropPos(m_lastDragPosition);
		Group *category = pair.first;
		int row = pair.second;
		if (category)
		{
			int internalRow = row - category->firstItemIndex;
			QLine line;
			if (internalRow >= category->numItems())
			{
				QRect toTheRightOfRect = visualRect(category->lastItem());
				line = QLine(toTheRightOfRect.topRight(), toTheRightOfRect.bottomRight());
			}
			else
			{
				QRect toTheLeftOfRect = visualRect(model()->index(row, 0));
				line = QLine(toTheLeftOfRect.topLeft(), toTheLeftOfRect.bottomLeft());
			}
			painter.save();
			painter.setPen(QPen(Qt::black, 3));
			painter.drawLine(line);
			painter.restore();
		}
	}
#endif
}

void GroupView::resizeEvent(QResizeEvent *event)
{
	int newItemsPerRow = calculateItemsPerRow();
	if(newItemsPerRow != m_currentItemsPerRow)
	{
		m_currentCursorColumn = -1;
		m_currentItemsPerRow = newItemsPerRow;
		updateGeometries();
	}
}

void GroupView::dragEnterEvent(QDragEnterEvent *event)
{
	if (!isDragEventAccepted(event))
	{
		return;
	}
	m_lastDragPosition = event->pos() + offset();
	viewport()->update();
	event->accept();
}

void GroupView::dragMoveEvent(QDragMoveEvent *event)
{
	if (!isDragEventAccepted(event))
	{
		return;
	}
	m_lastDragPosition = event->pos() + offset();
	viewport()->update();
	event->accept();
}

void GroupView::dragLeaveEvent(QDragLeaveEvent *event)
{
	m_lastDragPosition = QPoint();
	viewport()->update();
}

void GroupView::dropEvent(QDropEvent *event)
{
	m_lastDragPosition = QPoint();

	stopAutoScroll();
	setState(NoState);

	if (event->source() != this || !(event->possibleActions() & Qt::MoveAction))
	{
		return;
	}

	QPair<VisualGroup *, int> dropPos = rowDropPos(event->pos() + offset());
	const VisualGroup *category = dropPos.first;
	const int row = dropPos.second;

	if (row == -1)
	{
		viewport()->update();
		return;
	}

	const QString categoryText = category->text;
	if (model()->dropMimeData(event->mimeData(), Qt::MoveAction, row, 0, QModelIndex()))
	{
		model()->setData(model()->index(row, 0), categoryText,
						 GroupViewRoles::GroupRole);
		event->setDropAction(Qt::MoveAction);
		event->accept();
	}
	updateGeometries();
	viewport()->update();
}

void GroupView::startDrag(Qt::DropActions supportedActions)
{
	QModelIndexList indexes = selectionModel()->selectedIndexes();
	if(indexes.count() == 0)
		return;

	QMimeData *data = model()->mimeData(indexes);
	if (!data)
	{
		return;
	}
	QRect rect;
	QPixmap pixmap = renderToPixmap(indexes, &rect);
	//rect.translate(offset());
	// rect.adjust(horizontalOffset(), verticalOffset(), 0, 0);
	QDrag *drag = new QDrag(this);
	drag->setPixmap(pixmap);
	drag->setMimeData(data);
	Qt::DropAction defaultDropAction = Qt::IgnoreAction;
	if (this->defaultDropAction() != Qt::IgnoreAction &&
		(supportedActions & this->defaultDropAction()))
	{
		defaultDropAction = this->defaultDropAction();
	}
	if (drag->exec(supportedActions, defaultDropAction) == Qt::MoveAction)
	{
		const QItemSelection selection = selectionModel()->selection();

		for (auto it = selection.constBegin(); it != selection.constEnd(); ++it)
		{
			QModelIndex parent = (*it).parent();
			if ((*it).left() != 0)
			{
				continue;
			}
			if ((*it).right() != (model()->columnCount(parent) - 1))
			{
				continue;
			}
			int count = (*it).bottom() - (*it).top() + 1;
			model()->removeRows((*it).top(), count, parent);
		}
	}
}

QRect GroupView::visualRect(const QModelIndex &index) const
{
	return geometryRect(index).translated(-offset());
}

QRect GroupView::geometryRect(const QModelIndex &index) const
{
	if (!index.isValid() || isIndexHidden(index) || index.column() > 0)
	{
		return QRect();
	}

	int row = index.row();
	if(geometryCache.contains(row))
	{
		return *geometryCache[row];
	}

	const VisualGroup *cat = category(index);
	QPair<int, int> pos = cat->positionOf(index);
	int x = pos.first;
	// int y = pos.second;

	QRect out;
	out.setTop(cat->verticalPosition() + cat->headerHeight() + 5 + cat->rowTopOf(index));
	out.setLeft(m_spacing + x * (itemWidth() + m_spacing));
	out.setSize(itemDelegate()->sizeHint(viewOptions(), index));
	geometryCache.insert(row, new QRect(out));
	return out;
}

QModelIndex GroupView::indexAt(const QPoint &point) const
{
	for (int i = 0; i < model()->rowCount(); ++i)
	{
		QModelIndex index = model()->index(i, 0);
		if (visualRect(index).contains(point))
		{
			return index;
		}
	}
	return QModelIndex();
}

void GroupView::setSelection(const QRect &rect,
							 const QItemSelectionModel::SelectionFlags commands)
{
	for (int i = 0; i < model()->rowCount(); ++i)
	{
		QModelIndex index = model()->index(i, 0);
		QRect itemRect = visualRect(index);
		if (itemRect.intersects(rect))
		{
			selectionModel()->select(index, commands);
			update(itemRect.translated(-offset()));
		}
	}
}

QPixmap GroupView::renderToPixmap(const QModelIndexList &indices, QRect *r) const
{
	Q_ASSERT(r);
	auto paintPairs = draggablePaintPairs(indices, r);
	if (paintPairs.isEmpty())
	{
		return QPixmap();
	}
	QPixmap pixmap(r->size());
	pixmap.fill(Qt::transparent);
	QPainter painter(&pixmap);
	QStyleOptionViewItem option = viewOptions();
	option.state |= QStyle::State_Selected;
	for (int j = 0; j < paintPairs.count(); ++j)
	{
		option.rect = paintPairs.at(j).first.translated(-r->topLeft());
		const QModelIndex &current = paintPairs.at(j).second;
		itemDelegate()->paint(&painter, option, current);
	}
	return pixmap;
}

QList<QPair<QRect, QModelIndex>> GroupView::draggablePaintPairs(const QModelIndexList &indices,
																QRect *r) const
{
	Q_ASSERT(r);
	QRect &rect = *r;
	QList<QPair<QRect, QModelIndex>> ret;
	for (int i = 0; i < indices.count(); ++i)
	{
		const QModelIndex &index = indices.at(i);
		const QRect current = geometryRect(index);
		ret += qMakePair(current, index);
		rect |= current;
	}
	return ret;
}

bool GroupView::isDragEventAccepted(QDropEvent *event)
{
	return false;
}

QPair<VisualGroup *, int> GroupView::rowDropPos(const QPoint &pos)
{
	return qMakePair<VisualGroup*, int>(nullptr, -1);
}

QPoint GroupView::offset() const
{
	return QPoint(horizontalOffset(), verticalOffset());
}

QRegion GroupView::visualRegionForSelection(const QItemSelection &selection) const
{
	QRegion region;
	for (auto &range : selection)
	{
		int start_row = range.top();
		int end_row = range.bottom();
		for (int row = start_row; row <= end_row; ++row)
		{
			int start_column = range.left();
			int end_column = range.right();
			for (int column = start_column; column <= end_column; ++column)
			{
				QModelIndex index = model()->index(row, column, rootIndex());
				region += visualRect(index); // OK
			}
		}
	}
	return region;
}

QModelIndex GroupView::moveCursor(QAbstractItemView::CursorAction cursorAction,
								  Qt::KeyboardModifiers modifiers)
{
	auto current = currentIndex();
	if(!current.isValid())
	{
		return current;
	}
	auto cat = category(current);
	int group_index = m_groups.indexOf(cat);
	if(group_index < 0)
		return current;

	auto real_group = m_groups[group_index];
	int beginning_row = 0;
	for(auto group: m_groups)
	{
		if(group == real_group)
			break;
		beginning_row += group->numRows();
	}

	QPair<int, int> pos = cat->positionOf(current);
	int column = pos.first;
	int row = pos.second;
	if(m_currentCursorColumn < 0)
	{
		m_currentCursorColumn = column;
	}
	switch(cursorAction)
	{
		case MoveUp:
		{
			if(row == 0)
			{
				int prevgroupindex = group_index-1;
				while(prevgroupindex >= 0)
				{
					auto prevgroup = m_groups[prevgroupindex];
					if(prevgroup->collapsed)
					{
						prevgroupindex--;
						continue;
					}
					int newRow = prevgroup->numRows() - 1;
					int newRowSize = prevgroup->rows[newRow].size();
					int newColumn = m_currentCursorColumn;
					if (m_currentCursorColumn >= newRowSize)
					{
						newColumn = newRowSize - 1;
					}
					return prevgroup->rows[newRow][newColumn];
				}
			}
			else
			{
				int newRow = row - 1;
				int newRowSize = cat->rows[newRow].size();
				int newColumn = m_currentCursorColumn;
				if (m_currentCursorColumn >= newRowSize)
				{
					newColumn = newRowSize - 1;
				}
				return cat->rows[newRow][newColumn];
			}
			return current;
		}
		case MoveDown:
		{
			if(row == cat->rows.size() - 1)
			{
				int nextgroupindex = group_index+1;
				while (nextgroupindex < m_groups.size())
				{
					auto nextgroup = m_groups[nextgroupindex];
					if(nextgroup->collapsed)
					{
						nextgroupindex++;
						continue;
					}
					int newRowSize = nextgroup->rows[0].size();
					int newColumn = m_currentCursorColumn;
					if (m_currentCursorColumn >= newRowSize)
					{
						newColumn = newRowSize - 1;
					}
					return nextgroup->rows[0][newColumn];
				}
			}
			else
			{
				int newRow = row + 1;
				int newRowSize = cat->rows[newRow].size();
				int newColumn = m_currentCursorColumn;
				if (m_currentCursorColumn >= newRowSize)
				{
					newColumn = newRowSize - 1;
				}
				return cat->rows[newRow][newColumn];
			}
			return current;
		}
		case MoveLeft:
		{
			if(column > 0)
			{
				m_currentCursorColumn = column - 1;
				return cat->rows[row][column - 1];
			}
			// TODO: moving to previous line
			return current;
		}
		case MoveRight:
		{
			if(column < cat->rows[row].size() - 1)
			{
				m_currentCursorColumn = column + 1;
				return cat->rows[row][column + 1];
			}
			// TODO: moving to next line
			return current;
		}
		case MoveHome:
		{
			m_currentCursorColumn = 0;
			return cat->rows[row][0];
		}
		case MoveEnd:
		{
			auto last = cat->rows[row].size() - 1;
			m_currentCursorColumn = last;
			return cat->rows[row][last];
		}
		default:
			break;
	}
	return current;
}

int GroupView::horizontalOffset() const
{
	return horizontalScrollBar()->value();
}

int GroupView::verticalOffset() const
{
	return verticalScrollBar()->value();
}

void GroupView::scrollContentsBy(int dx, int dy)
{
	scrollDirtyRegion(dx, dy);
	viewport()->scroll(dx, dy);
}

void GroupView::scrollTo(const QModelIndex &index, ScrollHint hint)
{
	if (!index.isValid())
		return;

	const QRect rect = visualRect(index);
	if (hint == EnsureVisible && viewport()->rect().contains(rect))
	{
		viewport()->update(rect);
		return;
	}

	verticalScrollBar()->setValue(verticalScrollToValue(index, rect, hint));
}

int GroupView::verticalScrollToValue(const QModelIndex &index, const QRect &rect,
                                            QListView::ScrollHint hint) const
{
	const QRect area = viewport()->rect();
	const bool above = (hint == QListView::EnsureVisible && rect.top() < area.top());
	const bool below = (hint == QListView::EnsureVisible && rect.bottom() > area.bottom());

	int verticalValue = verticalScrollBar()->value();
	QRect adjusted = rect.adjusted(-spacing(), -spacing(), spacing(), spacing());
	if (hint == QListView::PositionAtTop || above)
		verticalValue += adjusted.top();
	else if (hint == QListView::PositionAtBottom || below)
		verticalValue += qMin(adjusted.top(), adjusted.bottom() - area.height() + 1);
	else if (hint == QListView::PositionAtCenter)
		verticalValue += adjusted.top() - ((area.height() - adjusted.height()) / 2);
	return verticalValue;
}
