#include "GroupView.h"
#include "AccessibleGroupView.h"
#include "AccessibleGroupView_p.h"

#include <qvariant.h>
#include <qaccessible.h>
#include <qheaderview.h>

QAccessibleInterface *groupViewAccessibleFactory(const QString &classname, QObject *object)
{
    QAccessibleInterface *iface = 0;
    if (!object || !object->isWidgetType())
        return iface;

    QWidget *widget = static_cast<QWidget*>(object);

    if (classname == QLatin1String("GroupView")) {
        iface = new AccessibleGroupView((GroupView *)widget);
    }
    return iface;
}


QAbstractItemView *AccessibleGroupView::view() const
{
    return qobject_cast<QAbstractItemView*>(object());
}

int AccessibleGroupView::logicalIndex(const QModelIndex &index) const
{
    if (!view()->model() || !index.isValid())
        return -1;
    return index.row() * (index.model()->columnCount()) + index.column();
}

AccessibleGroupView::AccessibleGroupView(QWidget *w)
    : QAccessibleObject(w)
{
    Q_ASSERT(view());
}

bool AccessibleGroupView::isValid() const
{
    return view();
}

AccessibleGroupView::~AccessibleGroupView()
{
    for (QAccessible::Id id : qAsConst(childToId)) {
        QAccessible::deleteAccessibleInterface(id);
    }
}

QAccessibleInterface *AccessibleGroupView::cellAt(int row, int column) const
{
    if (!view()->model()) {
        return 0;
    }

    QModelIndex index = view()->model()->index(row, column, view()->rootIndex());
    if (Q_UNLIKELY(!index.isValid())) {
        qWarning() << "AccessibleGroupView::cellAt: invalid index: " << index << " for " << view();
        return 0;
    }

    return child(logicalIndex(index));
}

QAccessibleInterface *AccessibleGroupView::caption() const
{
    return 0;
}

QString AccessibleGroupView::columnDescription(int column) const
{
    if (!view()->model())
        return QString();

    return view()->model()->headerData(column, Qt::Horizontal).toString();
}

int AccessibleGroupView::columnCount() const
{
    if (!view()->model())
        return 0;
    return 1;
}

int AccessibleGroupView::rowCount() const
{
    if (!view()->model())
        return 0;
    return view()->model()->rowCount();
}

int AccessibleGroupView::selectedCellCount() const
{
    if (!view()->selectionModel())
        return 0;
    return view()->selectionModel()->selectedIndexes().count();
}

int AccessibleGroupView::selectedColumnCount() const
{
    if (!view()->selectionModel())
        return 0;
    return view()->selectionModel()->selectedColumns().count();
}

int AccessibleGroupView::selectedRowCount() const
{
    if (!view()->selectionModel())
        return 0;
    return view()->selectionModel()->selectedRows().count();
}

QString AccessibleGroupView::rowDescription(int row) const
{
    if (!view()->model())
        return QString();
    return view()->model()->headerData(row, Qt::Vertical).toString();
}

QList<QAccessibleInterface *> AccessibleGroupView::selectedCells() const
{
    QList<QAccessibleInterface*> cells;
    if (!view()->selectionModel())
        return cells;
    const QModelIndexList selectedIndexes = view()->selectionModel()->selectedIndexes();
    cells.reserve(selectedIndexes.size());
    for (const QModelIndex &index : selectedIndexes)
        cells.append(child(logicalIndex(index)));
    return cells;
}

QList<int> AccessibleGroupView::selectedColumns() const
{
    if (!view()->selectionModel()) {
        return QList<int>();
    }

    const QModelIndexList selectedColumns = view()->selectionModel()->selectedColumns();

    QList<int> columns;
    columns.reserve(selectedColumns.size());
    for (const QModelIndex &index : selectedColumns) {
        columns.append(index.column());
    }

    return columns;
}

QList<int> AccessibleGroupView::selectedRows() const
{
    if (!view()->selectionModel()) {
        return QList<int>();
    }

    QList<int> rows;

    const QModelIndexList selectedRows = view()->selectionModel()->selectedRows();

    rows.reserve(selectedRows.size());
    for (const QModelIndex &index : selectedRows) {
        rows.append(index.row());
    }

    return rows;
}

QAccessibleInterface *AccessibleGroupView::summary() const
{
    return 0;
}

bool AccessibleGroupView::isColumnSelected(int column) const
{
    if (!view()->selectionModel()) {
        return false;
    }

    return view()->selectionModel()->isColumnSelected(column, QModelIndex());
}

bool AccessibleGroupView::isRowSelected(int row) const
{
    if (!view()->selectionModel()) {
        return false;
    }

    return view()->selectionModel()->isRowSelected(row, QModelIndex());
}

bool AccessibleGroupView::selectRow(int row)
{
    if (!view()->model() || !view()->selectionModel()) {
        return false;
    }
    QModelIndex index = view()->model()->index(row, 0, view()->rootIndex());

    if (!index.isValid() || view()->selectionBehavior() == QAbstractItemView::SelectColumns) {
        return false;
    }

    switch (view()->selectionMode()) {
        case QAbstractItemView::NoSelection: {
            return false;
        }
        case QAbstractItemView::SingleSelection: {
            if (view()->selectionBehavior() != QAbstractItemView::SelectRows && columnCount() > 1 )
                return false;
            view()->clearSelection();
            break;
        }
        case QAbstractItemView::ContiguousSelection: {
            if ((!row || !view()->selectionModel()->isRowSelected(row - 1, view()->rootIndex())) && !view()->selectionModel()->isRowSelected(row + 1, view()->rootIndex())) {
                view()->clearSelection();
            }
        break;
        }
        default: {
            break;
        }
    }

    view()->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    return true;
}

bool AccessibleGroupView::selectColumn(int column)
{
    if (!view()->model() || !view()->selectionModel()) {
        return false;
    }
    QModelIndex index = view()->model()->index(0, column, view()->rootIndex());

    if (!index.isValid() || view()->selectionBehavior() == QAbstractItemView::SelectRows) {
        return false;
    }

    switch (view()->selectionMode()) {
        case QAbstractItemView::NoSelection: {
            return false;
        }
        case QAbstractItemView::SingleSelection: {
            if (view()->selectionBehavior() != QAbstractItemView::SelectColumns && rowCount() > 1) {
                return false;
            }
            Q_FALLTHROUGH();
        }
        case QAbstractItemView::ContiguousSelection: {
            if ((!column || !view()->selectionModel()->isColumnSelected(column - 1, view()->rootIndex())) && !view()->selectionModel()->isColumnSelected(column + 1, view()->rootIndex())) {
                view()->clearSelection();
            }
            break;
        }
        default: {
            break;
        }
    }

    view()->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Columns);
    return true;
}

bool AccessibleGroupView::unselectRow(int row)
{
    if (!view()->model() || !view()->selectionModel()) {
        return false;
    }

    QModelIndex index = view()->model()->index(row, 0, view()->rootIndex());
    if (!index.isValid()) {
        return false;
    }

    QItemSelection selection(index, index);
    auto selectionModel = view()->selectionModel();

    switch (view()->selectionMode()) {
        case QAbstractItemView::SingleSelection:
            // no unselect
            if (selectedRowCount() == 1) {
                return false;
            }
            break;
        case QAbstractItemView::ContiguousSelection: {
            // no unselect
            if (selectedRowCount() == 1) {
                return false;
            }


            if ((!row || selectionModel->isRowSelected(row - 1, view()->rootIndex())) && selectionModel->isRowSelected(row + 1, view()->rootIndex())) {
                //If there are rows selected both up the current row and down the current rown,
                //the ones which are down the current row will be deselected
                selection = QItemSelection(index, view()->model()->index(rowCount() - 1, 0, view()->rootIndex()));
            }
        }
        default: {
            break;
        }
    }

    selectionModel->select(selection, QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
    return true;
}

bool AccessibleGroupView::unselectColumn(int column)
{
    auto model = view()->model();
    if (!model || !view()->selectionModel()) {
        return false;
    }

    QModelIndex index = model->index(0, column, view()->rootIndex());
    if (!index.isValid()) {
        return false;
    }

    QItemSelection selection(index, index);

    switch (view()->selectionMode()) {
        case QAbstractItemView::SingleSelection: {
            //In SingleSelection and ContiguousSelection once an item
            //is selected, there's no way for the user to unselect all items
            if (selectedColumnCount() == 1) {
                return false;
            }
            break;
        }
        case QAbstractItemView::ContiguousSelection:
            if (selectedColumnCount() == 1) {
                return false;
            }

            if ((!column || view()->selectionModel()->isColumnSelected(column - 1, view()->rootIndex()))
                && view()->selectionModel()->isColumnSelected(column + 1, view()->rootIndex())) {
                //If there are columns selected both at the left of the current row and at the right
                //of the current row, the ones which are at the right will be deselected
                selection = QItemSelection(index, model->index(0, columnCount() - 1, view()->rootIndex()));
            }
        default:
            break;
    }

    view()->selectionModel()->select(selection, QItemSelectionModel::Deselect | QItemSelectionModel::Columns);
    return true;
}

QAccessible::Role AccessibleGroupView::role() const
{
    return QAccessible::List;
}

QAccessible::State AccessibleGroupView::state() const
{
    return QAccessible::State();
}

QAccessibleInterface *AccessibleGroupView::childAt(int x, int y) const
{
    QPoint viewportOffset = view()->viewport()->mapTo(view(), QPoint(0,0));
    QPoint indexPosition = view()->mapFromGlobal(QPoint(x, y) - viewportOffset);
    // FIXME: if indexPosition < 0 in one coordinate, return header

    QModelIndex index = view()->indexAt(indexPosition);
    if (index.isValid()) {
        return child(logicalIndex(index));
    }
    return 0;
}

int AccessibleGroupView::childCount() const
{
    if (!view()->model()) {
        return 0;
    }
    return (view()->model()->rowCount()) * (view()->model()->columnCount());
}

int AccessibleGroupView::indexOfChild(const QAccessibleInterface *iface) const
{
    if (!view()->model())
        return -1;
    QAccessibleInterface *parent = iface->parent();
    if (parent->object() != view())
        return -1;

    Q_ASSERT(iface->role() != QAccessible::TreeItem); // should be handled by tree class
    if (iface->role() == QAccessible::Cell || iface->role() == QAccessible::ListItem) {
        const AccessibleGroupViewItem* cell = static_cast<const AccessibleGroupViewItem*>(iface);
        return logicalIndex(cell->m_index);
    } else if (iface->role() == QAccessible::Pane) {
        return 0; // corner button
    } else {
        qWarning() << "AccessibleGroupView::indexOfChild has a child with unknown role..." << iface->role() << iface->text(QAccessible::Name);
    }
    // FIXME: we are in denial of our children. this should stop.
    return -1;
}

QString AccessibleGroupView::text(QAccessible::Text t) const
{
    if (t == QAccessible::Description)
        return view()->accessibleDescription();
    return view()->accessibleName();
}

QRect AccessibleGroupView::rect() const
{
    if (!view()->isVisible())
        return QRect();
    QPoint pos = view()->mapToGlobal(QPoint(0, 0));
    return QRect(pos.x(), pos.y(), view()->width(), view()->height());
}

QAccessibleInterface *AccessibleGroupView::parent() const
{
    if (view() && view()->parent()) {
        if (qstrcmp("QComboBoxPrivateContainer", view()->parent()->metaObject()->className()) == 0) {
            return QAccessible::queryAccessibleInterface(view()->parent()->parent());
        }
        return QAccessible::queryAccessibleInterface(view()->parent());
    }
    return 0;
}

QAccessibleInterface *AccessibleGroupView::child(int logicalIndex) const
{
    if (!view()->model())
        return 0;

    auto id = childToId.constFind(logicalIndex);
    if (id != childToId.constEnd())
        return QAccessible::accessibleInterface(id.value());

    int columns = view()->model()->columnCount();

    int row = logicalIndex / columns;
    int column = logicalIndex % columns;

    QAccessibleInterface *iface = 0;

    QModelIndex index = view()->model()->index(row, column, view()->rootIndex());
    if (Q_UNLIKELY(!index.isValid())) {
        qWarning("AccessibleGroupView::child: Invalid index at: %d %d", row, column);
        return 0;
    }
    iface = new AccessibleGroupViewItem(view(), index);

    QAccessible::registerAccessibleInterface(iface);
    childToId.insert(logicalIndex, QAccessible::uniqueId(iface));
    return iface;
}

void *AccessibleGroupView::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::TableInterface)
       return static_cast<QAccessibleTableInterface*>(this);
   return 0;
}

void AccessibleGroupView::modelChange(QAccessibleTableModelChangeEvent *event)
{
    // if there is no cache yet, we don't update anything
    if (childToId.isEmpty())
        return;

    switch (event->modelChangeType()) {
        case QAccessibleTableModelChangeEvent::ModelReset:
            for (QAccessible::Id id : qAsConst(childToId))
                QAccessible::deleteAccessibleInterface(id);
            childToId.clear();
            break;

        // rows are inserted: move every row after that
        case QAccessibleTableModelChangeEvent::RowsInserted:
        case QAccessibleTableModelChangeEvent::ColumnsInserted: {

            ChildCache newCache;
            ChildCache::ConstIterator iter = childToId.constBegin();

            while (iter != childToId.constEnd()) {
                QAccessible::Id id = iter.value();
                QAccessibleInterface *iface = QAccessible::accessibleInterface(id);
                Q_ASSERT(iface);
                if (indexOfChild(iface) >= 0) {
                    newCache.insert(indexOfChild(iface), id);
                } else {
                    // ### This should really not happen,
                    // but it might if the view has a root index set.
                    // This needs to be fixed.
                    QAccessible::deleteAccessibleInterface(id);
                }
                ++iter;
            }
            childToId = newCache;
            break;
        }

        case QAccessibleTableModelChangeEvent::ColumnsRemoved:
        case QAccessibleTableModelChangeEvent::RowsRemoved: {
            ChildCache newCache;
            ChildCache::ConstIterator iter = childToId.constBegin();
            while (iter != childToId.constEnd()) {
                QAccessible::Id id = iter.value();
                QAccessibleInterface *iface = QAccessible::accessibleInterface(id);
                Q_ASSERT(iface);
                if (iface->role() == QAccessible::Cell || iface->role() == QAccessible::ListItem) {
                    Q_ASSERT(iface->tableCellInterface());
                    AccessibleGroupViewItem *cell = static_cast<AccessibleGroupViewItem*>(iface->tableCellInterface());
                    // Since it is a QPersistentModelIndex, we only need to check if it is valid
                    if (cell->m_index.isValid())
                        newCache.insert(indexOfChild(cell), id);
                    else
                        QAccessible::deleteAccessibleInterface(id);
                }
                ++iter;
            }
            childToId = newCache;
            break;
        }

        case QAccessibleTableModelChangeEvent::DataChanged:
            // nothing to do in this case
            break;
    }
}

// TABLE CELL

AccessibleGroupViewItem::AccessibleGroupViewItem(QAbstractItemView *view_, const QModelIndex &index_)
    : view(view_), m_index(index_)
{
    if (Q_UNLIKELY(!index_.isValid()))
        qWarning() << "AccessibleGroupViewItem::AccessibleGroupViewItem with invalid index: " << index_;
}

void *AccessibleGroupViewItem::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::TableCellInterface)
        return static_cast<QAccessibleTableCellInterface*>(this);
    if (t == QAccessible::ActionInterface)
        return static_cast<QAccessibleActionInterface*>(this);
    return 0;
}

int AccessibleGroupViewItem::columnExtent() const { return 1; }
int AccessibleGroupViewItem::rowExtent() const { return 1; }

QList<QAccessibleInterface*> AccessibleGroupViewItem::rowHeaderCells() const
{
    return {};
}

QList<QAccessibleInterface*> AccessibleGroupViewItem::columnHeaderCells() const
{
    return {};
}

int AccessibleGroupViewItem::columnIndex() const
{
    if (!isValid()) {
        return -1;
    }

    return m_index.column();
}

int AccessibleGroupViewItem::rowIndex() const
{
    if (!isValid()) {
        return -1;
    }

    return m_index.row();
}

bool AccessibleGroupViewItem::isSelected() const
{
    if (!isValid()) {
        return false;
    }

    return view->selectionModel()->isSelected(m_index);
}

QStringList AccessibleGroupViewItem::actionNames() const
{
    QStringList names;
    names << toggleAction();
    return names;
}

void AccessibleGroupViewItem::doAction(const QString& actionName)
{
    if (actionName == toggleAction()) {
        if (isSelected()) {
            unselectCell();
        }
        else {
            selectCell();
        }
    }
}

QStringList AccessibleGroupViewItem::keyBindingsForAction(const QString &) const
{
    return QStringList();
}


void AccessibleGroupViewItem::selectCell()
{
    if (!isValid()) {
        return;
    }
    QAbstractItemView::SelectionMode selectionMode = view->selectionMode();
    if (selectionMode == QAbstractItemView::NoSelection) {
        return;
    }

    Q_ASSERT(table());
    QAccessibleTableInterface *cellTable = table()->tableInterface();

    switch (view->selectionBehavior()) {
        case QAbstractItemView::SelectItems:
            break;
        case QAbstractItemView::SelectColumns:
            if (cellTable)
                cellTable->selectColumn(m_index.column());
            return;
        case QAbstractItemView::SelectRows:
            if (cellTable)
                cellTable->selectRow(m_index.row());
            return;
    }

    if (selectionMode == QAbstractItemView::SingleSelection) {
        view->clearSelection();
    }

    view->selectionModel()->select(m_index, QItemSelectionModel::Select);
}

void AccessibleGroupViewItem::unselectCell()
{
    if (!isValid())
        return;
    QAbstractItemView::SelectionMode selectionMode = view->selectionMode();
    if (selectionMode == QAbstractItemView::NoSelection)
        return;

    QAccessibleTableInterface *cellTable = table()->tableInterface();

    switch (view->selectionBehavior()) {
        case QAbstractItemView::SelectItems:
            break;
        case QAbstractItemView::SelectColumns:
            if (cellTable)
                cellTable->unselectColumn(m_index.column());
            return;
        case QAbstractItemView::SelectRows:
            if (cellTable)
                cellTable->unselectRow(m_index.row());
            return;
    }

    //If the mode is not MultiSelection or ExtendedSelection and only
    //one cell is selected it cannot be unselected by the user
    if ((selectionMode != QAbstractItemView::MultiSelection) && (selectionMode != QAbstractItemView::ExtendedSelection) && (view->selectionModel()->selectedIndexes().count() <= 1))
        return;

    view->selectionModel()->select(m_index, QItemSelectionModel::Deselect);
}

QAccessibleInterface *AccessibleGroupViewItem::table() const
{
    return QAccessible::queryAccessibleInterface(view);
}

QAccessible::Role AccessibleGroupViewItem::role() const
{
    return QAccessible::ListItem;
}

QAccessible::State AccessibleGroupViewItem::state() const
{
    QAccessible::State st;
    if (!isValid())
        return st;

    QRect globalRect = view->rect();
    globalRect.translate(view->mapToGlobal(QPoint(0,0)));
    if (!globalRect.intersects(rect()))
        st.invisible = true;

    if (view->selectionModel()->isSelected(m_index))
        st.selected = true;
    if (view->selectionModel()->currentIndex() == m_index)
        st.focused = true;
    if (m_index.model()->data(m_index, Qt::CheckStateRole).toInt() == Qt::Checked)
        st.checked = true;

    Qt::ItemFlags flags = m_index.flags();
    if (flags & Qt::ItemIsSelectable) {
        st.selectable = true;
        st.focusable = true;
        if (view->selectionMode() == QAbstractItemView::MultiSelection)
            st.multiSelectable = true;
        if (view->selectionMode() == QAbstractItemView::ExtendedSelection)
            st.extSelectable = true;
    }
    return st;
}


QRect AccessibleGroupViewItem::rect() const
{
    QRect r;
    if (!isValid())
        return r;
    r = view->visualRect(m_index);

    if (!r.isNull()) {
        r.translate(view->viewport()->mapTo(view, QPoint(0,0)));
        r.translate(view->mapToGlobal(QPoint(0, 0)));
    }
    return r;
}

QString AccessibleGroupViewItem::text(QAccessible::Text t) const
{
    QString value;
    if (!isValid())
        return value;
    QAbstractItemModel *model = view->model();
    switch (t) {
        case QAccessible::Name:
            value = model->data(m_index, Qt::AccessibleTextRole).toString();
            if (value.isEmpty())
                value = model->data(m_index, Qt::DisplayRole).toString();
            break;
        case QAccessible::Description:
            value = model->data(m_index, Qt::AccessibleDescriptionRole).toString();
            break;
        default:
            break;
    }
    return value;
}

void AccessibleGroupViewItem::setText(QAccessible::Text /*t*/, const QString &text)
{
    if (!isValid() || !(m_index.flags() & Qt::ItemIsEditable))
        return;
    view->model()->setData(m_index, text);
}

bool AccessibleGroupViewItem::isValid() const
{
    return view && view->model() && m_index.isValid();
}

QAccessibleInterface *AccessibleGroupViewItem::parent() const
{
    return QAccessible::queryAccessibleInterface(view);
}

QAccessibleInterface *AccessibleGroupViewItem::child(int) const
{
    return 0;
}
