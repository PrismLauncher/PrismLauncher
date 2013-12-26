#include "CategorizedView.h"

#include <QPainter>
#include <QApplication>
#include <QtMath>
#include <QDebug>
#include <QMouseEvent>
#include <QListView>
#include <QPersistentModelIndex>
#include <QDrag>
#include <QMimeData>

template<typename T>
bool listsIntersect(const QList<T> &l1, const QList<T> t2)
{
	foreach (const T &item, l1)
	{
		if (t2.contains(item))
		{
			return true;
		}
	}
	return false;
}

CategorizedView::Category::Category(const QString &text, CategorizedView *view)
	: view(view), text(text), collapsed(false)
{
}
CategorizedView::Category::Category(const CategorizedView::Category *other) :
	view(other->view), text(other->text), collapsed(other->collapsed), iconRect(other->iconRect), textRect(other->textRect)
{
}

void CategorizedView::Category::drawHeader(QPainter *painter, const int y)
{
	painter->save();

	int height = headerHeight() - 4;
	int collapseSize = height;

	// the icon
	iconRect = QRect(view->m_rightMargin + 2, 2 + y, collapseSize, collapseSize);
	painter->setPen(QPen(Qt::black, 1));
	painter->drawRect(iconRect);
	static const int margin = 2;
	QRect iconSubrect = iconRect.adjusted(margin, margin, -margin, -margin);
	int midX = iconSubrect.center().x();
	int midY = iconSubrect.center().y();
	if (collapsed)
	{
		painter->drawLine(midX, iconSubrect.top(), midX, iconSubrect.bottom());
	}
	painter->drawLine(iconSubrect.left(), midY, iconSubrect.right(), midY);

	// the text
	int textWidth = painter->fontMetrics().width(text);
	textRect = QRect(iconRect.right() + 4, y, textWidth, headerHeight());
	view->style()->drawItemText(painter, textRect, Qt::AlignHCenter | Qt::AlignVCenter, view->palette(), true, text);

	// the line
	painter->drawLine(textRect.right() + 4, y + headerHeight() / 2, view->contentWidth() - view->m_rightMargin, y + headerHeight() / 2);

	painter->restore();
}

int CategorizedView::Category::totalHeight() const
{
	return headerHeight() + 5 + contentHeight();
}
int CategorizedView::Category::headerHeight() const
{
	return qApp->fontMetrics().height() + 4;
}
int CategorizedView::Category::contentHeight() const
{
	if (collapsed)
	{
		return 0;
	}
	const int rows = qMax(1, qCeil((qreal)view->numItemsForCategory(this) / (qreal)view->itemsPerRow()));
	return view->itemSize().height() * rows;
}
QSize CategorizedView::Category::categoryTotalSize() const
{
	return QSize(view->contentWidth(), contentHeight());
}

CategorizedView::CategorizedView(QWidget *parent)
	: QListView(parent), m_leftMargin(5), m_rightMargin(5), m_categoryMargin(5)//, m_updatesDisabled(false), m_categoryEditor(0), m_editedCategory(0)
{
	setViewMode(IconMode);
	setMovement(Snap);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	setWordWrap(true);
	setDragDropMode(QListView::InternalMove);
	setAcceptDrops(true);

	m_cachedCategoryToIndexMapping.setMaxCost(50);
	m_cachedVisualRects.setMaxCost(50);
}

CategorizedView::~CategorizedView()
{
	qDeleteAll(m_categories);
	m_categories.clear();
}

void CategorizedView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
//	if (m_updatesDisabled)
//	{
//		return;
//	}

	QListView::dataChanged(topLeft, bottomRight, roles);

	if (roles.contains(CategoryRole))
	{
		updateGeometries();
		update();
	}
}
void CategorizedView::rowsInserted(const QModelIndex &parent, int start, int end)
{
//	if (m_updatesDisabled)
//	{
//		return;
//	}

	QListView::rowsInserted(parent, start, end);

	updateGeometries();
	update();
}
void CategorizedView::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
//	if (m_updatesDisabled)
//	{
//		return;
//	}

	QListView::rowsAboutToBeRemoved(parent, start, end);

	updateGeometries();
	update();
}

void CategorizedView::updateGeometries()
{
	QListView::updateGeometries();

	m_cachedItemSize = QSize();
	m_cachedCategoryToIndexMapping.clear();
	m_cachedVisualRects.clear();

	QMap<QString, Category *> cats;

	for (int i = 0; i < model()->rowCount(); ++i)
	{
		const QString category = model()->index(i, 0).data(CategoryRole).toString();
		if (!cats.contains(category))
		{
			Category *old = this->category(category);
			if (old)
			{
				cats.insert(category, new Category(old));
			}
			else
			{
				cats.insert(category, new Category(category, this));
			}
		}
	}

	/*if (m_editedCategory)
	{
		m_editedCategory = cats[m_editedCategory->text];
	}*/

	qDeleteAll(m_categories);
	m_categories = cats.values();

	update();
}

bool CategorizedView::isIndexHidden(const QModelIndex &index) const
{
	Category *cat = category(index);
	if (cat)
	{
		return cat->collapsed;
	}
	else
	{
		return false;
	}
}

CategorizedView::Category *CategorizedView::category(const QModelIndex &index) const
{
	return category(index.data(CategoryRole).toString());
}
CategorizedView::Category *CategorizedView::category(const QString &cat) const
{
	for (int i = 0; i < m_categories.size(); ++i)
	{
		if (m_categories.at(i)->text == cat)
		{
			return m_categories.at(i);
		}
	}
	return 0;
}
CategorizedView::Category *CategorizedView::categoryAt(const QPoint &pos) const
{
	for (int i = 0; i < m_categories.size(); ++i)
	{
		if (m_categories.at(i)->iconRect.contains(pos))
		{
			return m_categories.at(i);
		}
	}
	return 0;
}

int CategorizedView::numItemsForCategory(const CategorizedView::Category *category) const
{
	return itemsForCategory(category).size();
}
QList<QModelIndex> CategorizedView::itemsForCategory(const CategorizedView::Category *category) const
{
	if (!m_cachedCategoryToIndexMapping.contains(category))
	{
		QList<QModelIndex> *indices = new QList<QModelIndex>();
		for (int i = 0; i < model()->rowCount(); ++i)
		{
			if (model()->index(i, 0).data(CategoryRole).toString() == category->text)
			{
				indices->append(model()->index(i, 0));
			}
		}
		m_cachedCategoryToIndexMapping.insert(category, indices, indices->size());
	}
	return *m_cachedCategoryToIndexMapping.object(category);
}
QModelIndex CategorizedView::firstItemForCategory(const CategorizedView::Category *category) const
{
	QList<QModelIndex> indices = itemsForCategory(category);
	QModelIndex first;
	foreach (const QModelIndex &index, indices)
	{
		if (index.row() < first.row() || !first.isValid())
		{
			first = index;
		}
	}

	return first;
}
QModelIndex CategorizedView::lastItemForCategory(const CategorizedView::Category *category) const
{
	QList<QModelIndex> indices = itemsForCategory(category);
	QModelIndex last;
	foreach (const QModelIndex &index, indices)
	{
		if (index.row() > last.row() || !last.isValid())
		{
			last = index;
		}
	}

	return last;
}

int CategorizedView::categoryTop(const CategorizedView::Category *category) const
{
	int res = 0;
	const QList<Category *> cats = sortedCategories();
	for (int i = 0; i < cats.size(); ++i)
	{
		if (cats.at(i) == category)
		{
			break;
		}
		res += cats.at(i)->totalHeight() + m_categoryMargin;
	}
	return res;
}

int CategorizedView::itemsPerRow() const
{
	return qFloor((qreal)contentWidth() / (qreal)itemSize().width());
}
int CategorizedView::contentWidth() const
{
	return width() - m_leftMargin - m_rightMargin;
}

QList<CategorizedView::Category *> CategorizedView::sortedCategories() const
{
	QList<Category *> out = m_categories;
	qSort(out.begin(), out.end(), [](const Category *c1, const Category *c2) { return c1->text < c2->text; });
	return out;
}

QSize CategorizedView::itemSize(const QStyleOptionViewItem &option) const
{
	if (!m_cachedItemSize.isValid())
	{
		QModelIndex sample = model()->index(model()->rowCount() -1, 0);
		const QAbstractItemDelegate *delegate = itemDelegate();
		if (delegate)
		{
			m_cachedItemSize = delegate->sizeHint(option, sample);
			m_cachedItemSize.setWidth(m_cachedItemSize.width() + 20);
			m_cachedItemSize.setHeight(m_cachedItemSize.height() + 20);
		}
		else
		{
			m_cachedItemSize = QSize();
		}
	}
	return m_cachedItemSize;
}

void CategorizedView::mousePressEvent(QMouseEvent *event)
{
	//endCategoryEditor();

	QPoint pos = event->pos();
	QPersistentModelIndex index = indexAt(pos);

	m_pressedIndex = index;
	m_pressedAlreadySelected = selectionModel()->isSelected(m_pressedIndex);
	QItemSelectionModel::SelectionFlags command = selectionCommand(index, event);
	QPoint offset = QPoint(horizontalOffset(), verticalOffset());
	if (!(command & QItemSelectionModel::Current))
	{
		m_pressedPosition = pos + offset;
	}
	else if (!indexAt(m_pressedPosition - offset).isValid())
	{
		m_pressedPosition = visualRect(currentIndex()).center() + offset;
	}

	m_pressedCategory = categoryAt(m_pressedPosition);
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
		QRect rect(m_pressedPosition - offset, pos);
		if (command.testFlag(QItemSelectionModel::Toggle))
		{
			command &= ~QItemSelectionModel::Toggle;
			m_ctrlDragSelectionFlag = selectionModel()->isSelected(index) ? QItemSelectionModel::Deselect : QItemSelectionModel::Select;
			command |= m_ctrlDragSelectionFlag;
		}
		setSelection(rect, command);

		// signal handlers may change the model
		emit pressed(index);

	} else {
		// Forces a finalize() even if mouse is pressed, but not on a item
		selectionModel()->select(QModelIndex(), QItemSelectionModel::Select);
	}
}
void CategorizedView::mouseMoveEvent(QMouseEvent *event)
{
	QPoint topLeft;
	QPoint bottomRight = event->pos();

	if (state() == ExpandingState || state() == CollapsingState)
	{
		return;
	}

	if (state() == DraggingState)
	{
		topLeft = m_pressedPosition - QPoint(horizontalOffset(), verticalOffset());
		if ((topLeft - event->pos()).manhattanLength() > QApplication::startDragDistance())
		{
			m_pressedIndex = QModelIndex();
			startDrag(model()->supportedDragActions());
			setState(NoState);
			stopAutoScroll();
		}
		return;
	}

	QPersistentModelIndex index = indexAt(bottomRight);

	if (selectionMode() != SingleSelection)
	{
		topLeft = m_pressedPosition - QPoint(horizontalOffset(), verticalOffset());
	}
	else
	{
		topLeft = bottomRight;
	}

	if (m_pressedIndex.isValid()
			&& (state() != DragSelectingState)
			&& (event->buttons() != Qt::NoButton)
			&& !selectedIndexes().isEmpty())
	{
		setState(DraggingState);
		return;
	}

	if ((event->buttons() & Qt::LeftButton) && selectionModel())
	{
		setState(DragSelectingState);
		QItemSelectionModel::SelectionFlags command = selectionCommand(index, event);
		if (m_ctrlDragSelectionFlag != QItemSelectionModel::NoUpdate && command.testFlag(QItemSelectionModel::Toggle))
		{
			command &= ~QItemSelectionModel::Toggle;
			command |= m_ctrlDragSelectionFlag;
		}

		// Do the normalize ourselves, since QRect::normalized() is flawed
		QRect selectionRect = QRect(topLeft, bottomRight);
		setSelection(selectionRect, command);

		// set at the end because it might scroll the view
		if (index.isValid()
				&& (index != selectionModel()->currentIndex())
				&& (index.flags() & Qt::ItemIsEnabled))
		{
			selectionModel()->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
		}
	}
}
void CategorizedView::mouseReleaseEvent(QMouseEvent *event)
{
	QPoint pos = event->pos();
	QPersistentModelIndex index = indexAt(pos);

	bool click = (index == m_pressedIndex && index.isValid()) || (m_pressedCategory && m_pressedCategory == categoryAt(pos));

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
		if ((model()->flags(index) & Qt::ItemIsEnabled)
				&& style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick, &option, this))
		{
			emit activated(index);
		}
	}
}
void CategorizedView::mouseDoubleClickEvent(QMouseEvent *event)
{
	QModelIndex index = indexAt(event->pos());
	if (!index.isValid()
			|| !(index.flags() & Qt::ItemIsEnabled)
			|| (m_pressedIndex != index))
	{
		QMouseEvent me(QEvent::MouseButtonPress,
					   event->localPos(), event->windowPos(), event->screenPos(),
					   event->button(), event->buttons(), event->modifiers());
		mousePressEvent(&me);
		return;
	}
	// signal handlers may change the model
	QPersistentModelIndex persistent = index;
	emit doubleClicked(persistent);
}
void CategorizedView::paintEvent(QPaintEvent *event)
{
	QPainter painter(this->viewport());

	int y = 0;
	for (int i = 0; i < m_categories.size(); ++i)
	{
		Category *category = m_categories.at(i);
		category->drawHeader(&painter, y);
		y += category->totalHeight() + m_categoryMargin;
	}

	for (int i = 0; i < model()->rowCount(); ++i)
	{
		const QModelIndex index = model()->index(i, 0);
		if (isIndexHidden(index))
		{
			continue;
		}
		Qt::ItemFlags flags = index.flags();
		QStyleOptionViewItemV4 option(viewOptions());
		option.rect = visualRect(index);
		option.widget = this;
		option.features |= wordWrap() ? QStyleOptionViewItemV2::WrapText : QStyleOptionViewItemV2::None;
		if (flags & Qt::ItemIsSelectable)
		{
			option.state |= selectionModel()->isSelected(index) ? QStyle::State_Selected : QStyle::State_None;
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

	if (!m_lastDragPosition.isNull())
	{
		QPair<Category *, int> pair = rowDropPos(m_lastDragPosition);
		Category *category = pair.first;
		int row = pair.second;
		if (category)
		{
			int internalRow = row - firstItemForCategory(category).row();
			QLine line;
			if (internalRow >= numItemsForCategory(category))
			{
				QRect toTheRightOfRect = visualRect(lastItemForCategory(category));
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
}
void CategorizedView::resizeEvent(QResizeEvent *event)
{
	QListView::resizeEvent(event);

//	if (m_categoryEditor)
//	{
//		m_categoryEditor->resize(qMax(contentWidth() / 2, m_editedCategory->textRect.width()), m_categoryEditor->height());
//	}

	updateGeometries();
}

void CategorizedView::dragEnterEvent(QDragEnterEvent *event)
{
	if (!isDragEventAccepted(event))
	{
		return;
	}
	m_lastDragPosition = event->pos();
	viewport()->update();
	event->accept();
}
void CategorizedView::dragMoveEvent(QDragMoveEvent *event)
{
	if (!isDragEventAccepted(event))
	{
		return;
	}
	m_lastDragPosition = event->pos();
	viewport()->update();
	event->accept();
}
void CategorizedView::dragLeaveEvent(QDragLeaveEvent *event)
{
	m_lastDragPosition = QPoint();
	viewport()->update();
}
void CategorizedView::dropEvent(QDropEvent *event)
{
	m_lastDragPosition = QPoint();

	stopAutoScroll();
	setState(NoState);

	if (event->source() != this || !(event->possibleActions() & Qt::MoveAction))
	{
		return;
	}

	QPair<Category *, int> dropPos = rowDropPos(event->pos());
	const Category *category = dropPos.first;
	const int row = dropPos.second;

	if (row == -1)
	{
		viewport()->update();
		return;
	}

	const QString categoryText = category->text;
	if (model()->dropMimeData(event->mimeData(), Qt::MoveAction, row, 0, QModelIndex()))
	{
		model()->setData(model()->index(row, 0), categoryText, CategoryRole);
		event->setDropAction(Qt::MoveAction);
		event->accept();
	}
	updateGeometries();
	viewport()->update();
}

void CategorizedView::startDrag(Qt::DropActions supportedActions)
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
		rect.adjust(horizontalOffset(), verticalOffset(), 0, 0);
		QDrag *drag = new QDrag(this);
		drag->setPixmap(pixmap);
		drag->setMimeData(data);
		drag->setHotSpot(m_pressedPosition - rect.topLeft());
		Qt::DropAction defaultDropAction = Qt::IgnoreAction;
		if (this->defaultDropAction() != Qt::IgnoreAction && (supportedActions & this->defaultDropAction()))
		{
			defaultDropAction = this->defaultDropAction();
		}
		if (drag->exec(supportedActions, defaultDropAction) == Qt::MoveAction)
		{
			const QItemSelection selection = selectionModel()->selection();

			for (auto it = selection.constBegin(); it != selection.constEnd(); ++it) {
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

QRect CategorizedView::visualRect(const QModelIndex &index) const
{
	if (!index.isValid() || isIndexHidden(index) || index.column() > 0)
	{
		return QRect();
	}

	if (!m_cachedVisualRects.contains(index))
	{
		const Category *cat = category(index);
		QList<QModelIndex> indices = itemsForCategory(cat);
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

		QSize size = itemSize();

		QRect *out = new QRect;
		out->setTop(categoryTop(cat) + cat->headerHeight() + 5 + y * size.height());
		out->setLeft(x * size.width());
		out->setSize(size);

		m_cachedVisualRects.insert(index, out);
	}

	return *m_cachedVisualRects.object(index);
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
	connect(m_categoryEditor, &QLineEdit::returnPressed, this, &CategorizedView::endCategoryEditor);
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
		const_cast<QAbstractItemModel *>(index.model())->setData(index, m_categoryEditor->text(), CategoryRole);
	}
	m_updatesDisabled = false;
	delete m_categoryEditor;
	m_categoryEditor = 0;
	m_editedCategory = 0;
	updateGeometries();
}
*/

QModelIndex CategorizedView::indexAt(const QPoint &point) const
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
void CategorizedView::setSelection(const QRect &rect, const QItemSelectionModel::SelectionFlags commands)
{
	QItemSelection selection;
	for (int i = 0; i < model()->rowCount(); ++i)
	{
		QModelIndex index = model()->index(i, 0);
		if (visualRect(index).intersects(rect))
		{
			selection.merge(QItemSelection(index, index), QItemSelectionModel::Select);
		}
	}
	selectionModel()->select(selection, commands);
}

QPixmap CategorizedView::renderToPixmap(const QModelIndexList &indices, QRect *r) const
{
	Q_ASSERT(r);
	QList<QPair<QRect, QModelIndex> > paintPairs = draggablePaintPairs(indices, r);
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
QList<QPair<QRect, QModelIndex> > CategorizedView::draggablePaintPairs(const QModelIndexList &indices, QRect *r) const
{
	Q_ASSERT(r);
	QRect &rect = *r;
	const QRect viewportRect = viewport()->rect();
	QList<QPair<QRect, QModelIndex> > ret;
	for (int i = 0; i < indices.count(); ++i) {
		const QModelIndex &index = indices.at(i);
		const QRect current = visualRect(index);
		if (current.intersects(viewportRect)) {
			ret += qMakePair(current, index);
			rect |= current;
		}
	}
	rect &= viewportRect;
	return ret;
}

bool CategorizedView::isDragEventAccepted(QDropEvent *event)
{
	if (event->source() != this)
	{
		return false;
	}
	if (!listsIntersect(event->mimeData()->formats(), model()->mimeTypes()))
	{
		return false;
	}
	if (!model()->canDropMimeData(event->mimeData(), event->dropAction(), rowDropPos(event->pos()).second, 0, QModelIndex()))
	{
		return false;
	}
	return true;
}
QPair<CategorizedView::Category *, int> CategorizedView::rowDropPos(const QPoint &pos)
{
	// check that we aren't on a category header and calculate which category we're in
	Category *category = 0;
	{
		int y = 0;
		foreach (Category *cat, m_categories)
		{
			if (pos.y() > y && pos.y() < (y + cat->headerHeight()))
			{
				return qMakePair(nullptr, -1);
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
			return qMakePair(nullptr, -1);
		}
	}

	// calculate the internal column
	int internalColumn = -1;
	{
		const int itemWidth = itemSize().width();
		for (int i = 0, c = 0;
			 i < contentWidth();
			 i += itemWidth, ++c)
		{
			if (pos.x() > (i - itemWidth / 2) &&
					pos.x() < (i + itemWidth / 2))
			{
				internalColumn = c;
				break;
			}
		}
		if (internalColumn == -1)
		{
			return qMakePair(nullptr, -1);
		}
	}

	// calculate the internal row
	int internalRow = -1;
	{
		const int itemHeight = itemSize().height();
		const int top = categoryTop(category);
		for (int i = top + category->headerHeight(), r = 0;
			 i < top + category->totalHeight();
			 i += itemHeight, ++r)
		{
			if (pos.y() > i && pos.y() < (i + itemHeight))
			{
				internalRow = r;
				break;
			}
		}
		if (internalRow == -1)
		{
			return qMakePair(nullptr, -1);
		}
	}

	QList<QModelIndex> indices = itemsForCategory(category);

	// flaten the internalColumn/internalRow to one row
	int categoryRow = 0;
	{
		for (int i = 0; i < internalRow; ++i)
		{
			if ((i + 1) >= internalRow)
			{
				break;
			}
			categoryRow += itemsPerRow();
		}
		categoryRow += internalColumn;
	}

	// this is used if we're past the last item
	if (internalColumn >= qMin(itemsPerRow(), indices.size()))
	{
		return qMakePair(category, indices.last().row() + 1);
	}

	return qMakePair(category, indices.at(categoryRow).row());
}
