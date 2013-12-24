#include "CategorizedView.h"

#include <QPainter>
#include <QApplication>
#include <QtMath>
#include <QDebug>
#include <QMouseEvent>

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
	painter->drawText(textRect, text, QTextOption(Qt::AlignHCenter | Qt::AlignVCenter));

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

int CategorizedView::numItemsForCategory(const CategorizedView::Category *category) const
{
	return itemsForCategory(category).size();
}
QList<QModelIndex> CategorizedView::itemsForCategory(const CategorizedView::Category *category) const
{
	QList<QModelIndex> indices;
	for (int i = 0; i < model()->rowCount(); ++i)
	{
		if (model()->index(i, 0).data(CategoryRole).toString() == category->text)
		{
			indices += model()->index(i, 0);
		}
	}
	return indices;
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

bool CategorizedView::lessThanCategoryPointer(const CategorizedView::Category *c1, const CategorizedView::Category *c2)
{
	return c1->text < c2->text;
}
QList<CategorizedView::Category *> CategorizedView::sortedCategories() const
{
	QList<Category *> out = m_categories;
	qSort(out.begin(), out.end(), &CategorizedView::lessThanCategoryPointer);
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

	if (event->buttons() & Qt::LeftButton)
	{
		foreach (Category *category, m_categories)
		{
			if (category->iconRect.contains(event->pos()))
			{
				category->collapsed = !category->collapsed;
				updateGeometries();
				viewport()->update();
				event->accept();
				return;
			}
		}

		for (int i = 0; i < model()->rowCount(); ++i)
		{
			QModelIndex index = model()->index(i, 0);
			if (visualRect(index).contains(event->pos()))
			{
				selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
				event->accept();
				return;
			}
		}
	}

	QListView::mousePressEvent(event);
}
void CategorizedView::mouseMoveEvent(QMouseEvent *event)
{
	if (event->buttons() & Qt::LeftButton)
	{
		for (int i = 0; i < model()->rowCount(); ++i)
		{
			QModelIndex index = model()->index(i, 0);
			if (visualRect(index).contains(event->pos()))
			{
				selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
				event->accept();
				return;
			}
		}
	}

	QListView::mouseMoveEvent(event);
}
void CategorizedView::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->buttons() & Qt::LeftButton)
	{
		for (int i = 0; i < model()->rowCount(); ++i)
		{
			QModelIndex index = model()->index(i, 0);
			if (visualRect(index).contains(event->pos()))
			{
				selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
				event->accept();
				return;
			}
		}
	}

	QListView::mouseReleaseEvent(event);
}
void CategorizedView::mouseDoubleClickEvent(QMouseEvent *event)
{
	/*endCategoryEditor();

	foreach (Category *category, m_categories)
	{
		if (category->textRect.contains(event->pos()) && m_categoryEditor == 0)
		{
			startCategoryEditor(category);
			event->accept();
			return;
		}
	}*/

	QListView::mouseDoubleClickEvent(event);
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
	// TODO
}
void CategorizedView::dragMoveEvent(QDragMoveEvent *event)
{
	// TODO
}
void CategorizedView::dragLeaveEvent(QDragLeaveEvent *event)
{
	// TODO
}
void CategorizedView::dropEvent(QDropEvent *event)
{
	stopAutoScroll();
	setState(NoState);

	if (event->source() != this || !(event->possibleActions() & Qt::MoveAction))
	{
		return;
	}

	// check that we aren't on a category header and calculate which category we're in
	Category *category = 0;
	{
		int y = 0;
		foreach (Category *cat, m_categories)
		{
			if (event->pos().y() > y && event->pos().y() < (y + cat->headerHeight()))
			{
				viewport()->update();
				return;
			}
			y += cat->totalHeight() + m_categoryMargin;
			if (event->pos().y() < y)
			{
				category = cat;
				break;
			}
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
			if (event->pos().x() > (i - itemWidth / 2) &&
					event->pos().x() < (i + itemWidth / 2))
			{
				internalColumn = c;
				break;
			}
		}
		if (internalColumn == -1)
		{
			viewport()->update();
			return;
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
			if (event->pos().y() > i && event->pos().y() < (i + itemHeight))
			{
				internalRow = r;
				break;
			}
		}
		if (internalRow == -1)
		{
			viewport()->update();
			return;
		}
	}

	QList<QModelIndex> indices = itemsForCategory(category);

	// flaten the internalColumn/internalRow to one row
	int categoryRow;
	{
		for (int i = 0; i < internalRow; ++i)
		{
			if (i == internalRow)
			{
				break;
			}
			categoryRow += itemsPerRow();
		}
		categoryRow += internalColumn;
	}

	int row = indices.at(categoryRow).row();
	if (model()->dropMimeData(event->mimeData(), Qt::MoveAction, row, 0, QModelIndex()))
	{
		event->setDropAction(Qt::MoveAction);
		event->accept();
	}
	updateGeometries();
}

bool lessThanQModelIndex(const QModelIndex &i1, const QModelIndex &i2)
{
	return i1.data() < i2.data();
}
QRect CategorizedView::visualRect(const QModelIndex &index) const
{
	if (!index.isValid() || isIndexHidden(index) || index.column() > 0)
	{
		return QRect();
	}

	const Category *cat = category(index);
	QList<QModelIndex> indices = itemsForCategory(cat);
	qSort(indices.begin(), indices.end(), &lessThanQModelIndex);
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

	QRect out;
	out.setTop(categoryTop(cat) + cat->headerHeight() + 5 + y * size.height());
	out.setLeft(x * size.width());
	out.setSize(size);

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
