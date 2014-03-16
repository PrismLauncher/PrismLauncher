#include "GroupView.h"

#include <QPainter>
#include <QApplication>
#include <QtMath>
#include <QDebug>
#include <QMouseEvent>
#include <QListView>
#include <QPersistentModelIndex>
#include <QDrag>
#include <QMimeData>
#include <QScrollBar>

#include "Group.h"

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
	: QAbstractItemView(parent), m_leftMargin(5), m_rightMargin(5), m_bottomMargin(5),
	  m_categoryMargin(5) //, m_updatesDisabled(false), m_categoryEditor(0), m_editedCategory(0)
{
	// setViewMode(IconMode);
	// setMovement(Snap);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	// setWordWrap(true);
	// setDragDropMode(QListView::InternalMove);
	setAcceptDrops(true);
	m_spacing = 5;
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

void GroupView::updateGeometries()
{
	int previousScroll = verticalScrollBar()->value();

	QMap<QString, Group *> cats;

	for (int i = 0; i < model()->rowCount(); ++i)
	{
		const QString groupName =
			model()->index(i, 0).data(GroupViewRoles::GroupRole).toString();
		if (!cats.contains(groupName))
		{
			Group *old = this->category(groupName);
			if (old)
			{
				cats.insert(groupName, new Group(old));
			}
			else
			{
				cats.insert(groupName, new Group(groupName, this));
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
	Group *cat = category(index);
	if (cat)
	{
		return cat->collapsed;
	}
	else
	{
		return false;
	}
}

Group *GroupView::category(const QModelIndex &index) const
{
	return category(index.data(GroupViewRoles::GroupRole).toString());
}

Group *GroupView::category(const QString &cat) const
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

Group *GroupView::categoryAt(const QPoint &pos) const
{
	for (auto group : m_groups)
	{
		if(group->hitScan(pos) & Group::CheckboxHit)
		{
			return group;
		}
	}
	return nullptr;
}

int GroupView::itemsPerRow() const
{
	return qFloor((qreal)(contentWidth()) / (qreal)(itemWidth() + m_spacing));
}

int GroupView::contentWidth() const
{
	return width() - m_leftMargin - m_rightMargin;
}

int GroupView::itemWidth() const
{
	return itemDelegate()
		->sizeHint(viewOptions(), model()->index(model()->rowCount() - 1, 0))
		.width();
}

int GroupView::categoryRowHeight(const QModelIndex &index) const
{
	QModelIndexList indices;
	int internalRow = categoryInternalPosition(index).second;
	for (auto &i : category(index)->items())
	{
		if (categoryInternalPosition(i).second == internalRow)
		{
			indices.append(i);
		}
	}

	int largestHeight = 0;
	for (auto &i : indices)
	{
		largestHeight =
			qMax(largestHeight, itemDelegate()->sizeHint(viewOptions(), i).height());
	}
	return largestHeight + m_spacing;
}

QPair<int, int> GroupView::categoryInternalPosition(const QModelIndex &index) const
{
	QList<QModelIndex> indices = category(index)->items();
	int x = 0;
	int y = 0;
	const int perRow = itemsPerRow();
	for (int i = 0; i < indices.size(); ++i)
	{
		if (indices.at(i) == index)
		{
			break;
		}
		++x;
		if (x == perRow)
		{
			x = 0;
			++y;
		}
	}
	return qMakePair(x, y);
}

int GroupView::categoryInternalRowTop(const QModelIndex &index) const
{
	Group *cat = category(index);
	int categoryInternalRow = categoryInternalPosition(index).second;
	int result = 0;
	for (int i = 0; i < categoryInternalRow; ++i)
	{
		result += cat->rowHeights.at(i);
	}
	return result;
}

int GroupView::itemHeightForCategoryRow(const Group *category, const int internalRow) const
{
	for (auto &i : category->items())
	{
		QPair<int, int> pos = categoryInternalPosition(i);
		if (pos.second == internalRow)
		{
			return categoryRowHeight(i);
		}
	}
	return -1;
}

void GroupView::mousePressEvent(QMouseEvent *event)
{
	// endCategoryEditor();

	QPoint visualPos = event->pos();
	QPoint geometryPos = event->pos() + offset();
	
	QPersistentModelIndex index = indexAt(visualPos);

	m_pressedIndex = index;
	m_pressedAlreadySelected = selectionModel()->isSelected(m_pressedIndex);
	QItemSelectionModel::SelectionFlags selection_flags = selectionCommand(index, event);
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
		// we disable scrollTo for mouse press so the item doesn't change position
		// when the user is interacting with it (ie. clicking on it)
		bool autoScroll = hasAutoScroll();
		setAutoScroll(false);
		selectionModel()->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
		setAutoScroll(autoScroll);
		QRect rect(geometryPos, geometryPos);
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

		setSelection(QRect(geometryPos, geometryPos), QItemSelectionModel::ClearAndSelect);
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

	bool isRight = event->button() == Qt::RightButton;

	if((index == m_pressedIndex && index.isValid()) && isRight)
	{
		emit customContextMenuRequested(event->pos());
		return;
	}

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
		Group *category = m_groups.at(i);
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
	// QListView::resizeEvent(event);

	//	if (m_categoryEditor)
	//	{
	//		m_categoryEditor->resize(qMax(contentWidth() / 2,
	// m_editedCategory->textRect.width()),
	// m_categoryEditor->height());
	//	}

	updateGeometries();
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

	QPair<Group *, int> dropPos = rowDropPos(event->pos() + offset());
	const Group *category = dropPos.first;
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
	if (indexes.count() > 0)
	{
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

	const Group *cat = category(index);
	QPair<int, int> pos = categoryInternalPosition(index);
	int x = pos.first;
	// int y = pos.second;

	QRect out;
	out.setTop(cat->verticalPosition() + cat->headerHeight() + 5 + categoryInternalRowTop(index));
	out.setLeft(m_spacing + x * (itemWidth() + m_spacing));
	out.setSize(itemDelegate()->sizeHint(viewOptions(), index));

	return out;
}

/*
void CategorizedView::startCategoryEditor(Category *category)
{
	if (m_categoryEditor != 0)
	{
		return;
	}
	m_editedCategory = category;
	m_categoryEditor = new QLineEdit(m_editedCategory->text, this);
	QRect rect = m_editedCategory->textRect;
	rect.setWidth(qMax(contentWidth() / 2, rect.width()));
	m_categoryEditor->setGeometry(rect);
	m_categoryEditor->show();
	m_categoryEditor->setFocus();
	connect(m_categoryEditor, &QLineEdit::returnPressed, this,
&CategorizedView::endCategoryEditor);
}

void CategorizedView::endCategoryEditor()
{
	if (m_categoryEditor == 0)
	{
		return;
	}
	m_editedCategory->text = m_categoryEditor->text();
	m_updatesDisabled = true;
	foreach (const QModelIndex &index, itemsForCategory(m_editedCategory))
	{
		const_cast<QAbstractItemModel *>(index.model())->setData(index,
m_categoryEditor->text(), CategoryRole);
	}
	m_updatesDisabled = false;
	delete m_categoryEditor;
	m_categoryEditor = 0;
	m_editedCategory = 0;
	updateGeometries();
}
*/

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

// FIXME: is rect supposed to be geometry or visual coords?
void GroupView::setSelection(const QRect &rect,
							 const QItemSelectionModel::SelectionFlags commands)
{
	for (int i = 0; i < model()->rowCount(); ++i)
	{
		QModelIndex index = model()->index(i, 0);
		QRect itemRect = geometryRect(index);
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
	if (event->source() != this)
	{
		return false;
	}
	if (!listsIntersect(event->mimeData()->formats(), model()->mimeTypes()))
	{
		return false;
	}
	if (!model()->canDropMimeData(event->mimeData(), event->dropAction(),
								  rowDropPos(event->pos()).second, 0, QModelIndex()))
	{
		return false;
	}
	return true;
}

QPair<Group *, int> GroupView::rowDropPos(const QPoint &pos)
{
	// check that we aren't on a category header and calculate which category we're in
	Group *category = 0;
	{
		int y = 0;
		for (auto cat : m_groups)
		{
			if (pos.y() > y && pos.y() < (y + cat->headerHeight()))
			{
				return qMakePair<Group*, int>(nullptr, -1);
			}
			y += cat->totalHeight() + m_categoryMargin;
			if (pos.y() < y)
			{
				category = cat;
				break;
			}
		}
		if (category == 0)
		{
			return qMakePair<Group*, int>(nullptr, -1);
		}
	}

	QList<QModelIndex> indices = category->items();

	// calculate the internal column
	int internalColumn = -1;
	{
		const int itemWidth = this->itemWidth();
		if (pos.x() >= (itemWidth * itemsPerRow()))
		{
			internalColumn = itemsPerRow();
		}
		else
		{
			for (int i = 0, c = 0; i < contentWidth(); i += itemWidth + 10 /*spacing()*/, ++c)
			{
				if (pos.x() > (i - itemWidth / 2) && pos.x() <= (i + itemWidth / 2))
				{
					internalColumn = c;
					break;
				}
			}
		}
		if (internalColumn == -1)
		{
			return qMakePair<Group*, int>(nullptr, -1);
		}
	}

	// calculate the internal row
	int internalRow = -1;
	{
		// FIXME rework the drag and drop code
		const int top = category->verticalPosition();
		for (int r = 0, h = top; r < category->numRows();
			 h += itemHeightForCategoryRow(category, r), ++r)
		{
			if (pos.y() > h && pos.y() < (h + itemHeightForCategoryRow(category, r)))
			{
				internalRow = r;
				break;
			}
		}
		if (internalRow == -1)
		{
			return qMakePair<Group*, int>(nullptr, -1);
		}
		// this happens if we're in the margin between a one category and another
		// categories header
		if (internalRow > (indices.size() / itemsPerRow()))
		{
			return qMakePair<Group*, int>(nullptr, -1);
		}
	}

	// flaten the internalColumn/internalRow to one row
	int categoryRow = internalRow * itemsPerRow() + internalColumn;

	// this is used if we're past the last item
	if (categoryRow >= indices.size())
	{
		return qMakePair(category, indices.last().row() + 1);
	}

	return qMakePair(category, indices.at(categoryRow).row());
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
		qDebug() << "model row: invalid";
		return current;
	}
	qDebug() << "model row: " << current.row();
	auto cat = category(current);
	int i = m_groups.indexOf(cat);
	if(i >= 0)
	{
		// this is a pile of something foul
		auto real_group = m_groups[i];
		int beginning_row = 0;
		for(auto group: m_groups)
		{
			if(group == real_group)
				break;
			beginning_row += group->numRows();
		}
		qDebug() << "category: " << real_group->text;
		QPair<int, int> pos = categoryInternalPosition(current);
		int row = beginning_row + pos.second;
		qDebug() << "row: " << row;
		qDebug() << "column: " << pos.first;
	}
	return current;
}
