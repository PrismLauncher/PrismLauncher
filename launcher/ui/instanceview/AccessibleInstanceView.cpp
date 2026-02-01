#include "AccessibleInstanceView.h"
#include "AccessibleInstanceView_p.h"
#include "InstanceView.h"

#ifndef QT_NO_ACCESSIBILITY

QAccessibleInterface* groupViewAccessibleFactory(const QString& classname, QObject* object)
{
    QAccessibleInterface* iface = 0;
    if (!object || !object->isWidgetType())
        return iface;

    QWidget* widget = static_cast<QWidget*>(object);

    if (classname == QLatin1String("InstanceView")) {
        iface = new AccessibleInstanceView((InstanceView*)widget);
    }
    return iface;
}

QAbstractItemView* AccessibleInstanceView::view() const
{
    return qobject_cast<QAbstractItemView*>(object());
}

int AccessibleInstanceView::logicalIndex(const QModelIndex& index) const
{
    if (!view()->model() || !index.isValid())
        return -1;
    return index.row() * (index.model()->columnCount()) + index.column();
}

AccessibleInstanceView::AccessibleInstanceView(QWidget* w) : QAccessibleObject(w)
{
    Q_ASSERT(view());
}

bool AccessibleInstanceView::isValid() const
{
    return view();
}

AccessibleInstanceView::~AccessibleInstanceView()
{
    for (QAccessible::Id id : childToId) {
        QAccessible::deleteAccessibleInterface(id);
    }
}

QAccessibleInterface* AccessibleInstanceView::cellAt(int row, int column) const
{
    if (!view()->model()) {
        return 0;
    }

    QModelIndex index = view()->model()->index(row, column, view()->rootIndex());
    if (Q_UNLIKELY(!index.isValid())) {
        qWarning() << "AccessibleInstanceView::cellAt: invalid index:" << index << "for" << view();
        return 0;
    }

    return child(logicalIndex(index));
}

QAccessibleInterface* AccessibleInstanceView::caption() const
{
    return 0;
}

QString AccessibleInstanceView::columnDescription(int column) const
{
    if (!view()->model())
        return QString();

    return view()->model()->headerData(column, Qt::Horizontal).toString();
}

int AccessibleInstanceView::columnCount() const
{
    if (!view()->model())
        return 0;
    return 1;
}

int AccessibleInstanceView::rowCount() const
{
    if (!view()->model())
        return 0;
    return view()->model()->rowCount();
}

int AccessibleInstanceView::selectedCellCount() const
{
    if (!view()->selectionModel())
        return 0;
    return view()->selectionModel()->selectedIndexes().count();
}

int AccessibleInstanceView::selectedColumnCount() const
{
    if (!view()->selectionModel())
        return 0;
    return view()->selectionModel()->selectedColumns().count();
}

int AccessibleInstanceView::selectedRowCount() const
{
    if (!view()->selectionModel())
        return 0;
    return view()->selectionModel()->selectedRows().count();
}

QString AccessibleInstanceView::rowDescription(int row) const
{
    if (!view()->model())
        return QString();
    return view()->model()->headerData(row, Qt::Vertical).toString();
}

QList<QAccessibleInterface*> AccessibleInstanceView::selectedCells() const
{
    QList<QAccessibleInterface*> cells;
    if (!view()->selectionModel())
        return cells;
    const QModelIndexList selectedIndexes = view()->selectionModel()->selectedIndexes();
    cells.reserve(selectedIndexes.size());
    for (const QModelIndex& index : selectedIndexes)
        cells.append(child(logicalIndex(index)));
    return cells;
}

QList<int> AccessibleInstanceView::selectedColumns() const
{
    if (!view()->selectionModel()) {
        return QList<int>();
    }

    const QModelIndexList selectedColumns = view()->selectionModel()->selectedColumns();

    QList<int> columns;
    columns.reserve(selectedColumns.size());
    for (const QModelIndex& index : selectedColumns) {
        columns.append(index.column());
    }

    return columns;
}

QList<int> AccessibleInstanceView::selectedRows() const
{
    if (!view()->selectionModel()) {
        return QList<int>();
    }

    QList<int> rows;

    const QModelIndexList selectedRows = view()->selectionModel()->selectedRows();

    rows.reserve(selectedRows.size());
    for (const QModelIndex& index : selectedRows) {
        rows.append(index.row());
    }

    return rows;
}

QAccessibleInterface* AccessibleInstanceView::summary() const
{
    return 0;
}

bool AccessibleInstanceView::isColumnSelected(int column) const
{
    if (!view()->selectionModel()) {
        return false;
    }

    return view()->selectionModel()->isColumnSelected(column, QModelIndex());
}

bool AccessibleInstanceView::isRowSelected(int row) const
{
    if (!view()->selectionModel()) {
        return false;
    }

    return view()->selectionModel()->isRowSelected(row, QModelIndex());
}

bool AccessibleInstanceView::selectRow(int row)
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
            if (view()->selectionBehavior() != QAbstractItemView::SelectRows && columnCount() > 1)
                return false;
            view()->clearSelection();
            break;
        }
        case QAbstractItemView::ContiguousSelection: {
            if ((!row || !view()->selectionModel()->isRowSelected(row - 1, view()->rootIndex())) &&
                !view()->selectionModel()->isRowSelected(row + 1, view()->rootIndex())) {
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

bool AccessibleInstanceView::selectColumn(int column)
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
        }
        /* fallthrough */
        case QAbstractItemView::ContiguousSelection: {
            if ((!column || !view()->selectionModel()->isColumnSelected(column - 1, view()->rootIndex())) &&
                !view()->selectionModel()->isColumnSelected(column + 1, view()->rootIndex())) {
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

bool AccessibleInstanceView::unselectRow(int row)
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

            if ((!row || selectionModel->isRowSelected(row - 1, view()->rootIndex())) &&
                selectionModel->isRowSelected(row + 1, view()->rootIndex())) {
                // If there are rows selected both up the current row and down the current rown,
                // the ones which are down the current row will be deselected
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

bool AccessibleInstanceView::unselectColumn(int column)
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
            // In SingleSelection and ContiguousSelection once an item
            // is selected, there's no way for the user to unselect all items
            if (selectedColumnCount() == 1) {
                return false;
            }
            break;
        }
        case QAbstractItemView::ContiguousSelection:
            if (selectedColumnCount() == 1) {
                return false;
            }

            if ((!column || view()->selectionModel()->isColumnSelected(column - 1, view()->rootIndex())) &&
                view()->selectionModel()->isColumnSelected(column + 1, view()->rootIndex())) {
                // If there are columns selected both at the left of the current row and at the right
                // of the current row, the ones which are at the right will be deselected
                selection = QItemSelection(index, model->index(0, columnCount() - 1, view()->rootIndex()));
            }
        default:
            break;
    }

    view()->selectionModel()->select(selection, QItemSelectionModel::Deselect | QItemSelectionModel::Columns);
    return true;
}

QAccessible::Role AccessibleInstanceView::role() const
{
    return QAccessible::List;
}

QAccessible::State AccessibleInstanceView::state() const
{
    return QAccessible::State();
}

QAccessibleInterface* AccessibleInstanceView::childAt(int x, int y) const
{
    QPoint viewportOffset = view()->viewport()->mapTo(view(), QPoint(0, 0));
    QPoint indexPosition = view()->mapFromGlobal(QPoint(x, y) - viewportOffset);
    // FIXME: if indexPosition < 0 in one coordinate, return header

    QModelIndex index = view()->indexAt(indexPosition);
    if (index.isValid()) {
        return child(logicalIndex(index));
    }
    return 0;
}

int AccessibleInstanceView::childCount() const
{
    if (!view()->model()) {
        return 0;
    }
    return (view()->model()->rowCount()) * (view()->model()->columnCount());
}

int AccessibleInstanceView::indexOfChild(const QAccessibleInterface* iface) const
{
    if (!view()->model())
        return -1;
    QAccessibleInterface* parent = iface->parent();
    if (parent->object() != view())
        return -1;

    Q_ASSERT(iface->role() != QAccessible::TreeItem);  // should be handled by tree class
    if (iface->role() == QAccessible::Cell || iface->role() == QAccessible::ListItem) {
        const AccessibleInstanceViewItem* cell = static_cast<const AccessibleInstanceViewItem*>(iface);
        return logicalIndex(cell->m_index);
    } else if (iface->role() == QAccessible::Pane) {
        return 0;  // corner button
    } else {
        qWarning() << "AccessibleInstanceView::indexOfChild has a child with unknown role..." << iface->role()
                   << iface->text(QAccessible::Name);
    }
    // FIXME: we are in denial of our children. this should stop.
    return -1;
}

QString AccessibleInstanceView::text(QAccessible::Text t) const
{
    if (t == QAccessible::Description)
        return view()->accessibleDescription();
    return view()->accessibleName();
}

QRect AccessibleInstanceView::rect() const
{
    if (!view()->isVisible())
        return QRect();
    QPoint pos = view()->mapToGlobal(QPoint(0, 0));
    return QRect(pos.x(), pos.y(), view()->width(), view()->height());
}

QAccessibleInterface* AccessibleInstanceView::parent() const
{
    if (view() && view()->parent()) {
        if (qstrcmp("QComboBoxPrivateContainer", view()->parent()->metaObject()->className()) == 0) {
            return QAccessible::queryAccessibleInterface(view()->parent()->parent());
        }
        return QAccessible::queryAccessibleInterface(view()->parent());
    }
    return 0;
}

QAccessibleInterface* AccessibleInstanceView::child(int logicalIndex) const
{
    if (!view()->model())
        return 0;

    auto id = childToId.constFind(logicalIndex);
    if (id != childToId.constEnd())
        return QAccessible::accessibleInterface(id.value());

    int columns = view()->model()->columnCount();

    int row = logicalIndex / columns;
    int column = logicalIndex % columns;

    QAccessibleInterface* iface = 0;

    QModelIndex index = view()->model()->index(row, column, view()->rootIndex());
    if (Q_UNLIKELY(!index.isValid())) {
        qWarning("AccessibleInstanceView::child: Invalid index at: %d %d", row, column);
        return 0;
    }
    iface = new AccessibleInstanceViewItem(view(), index);

    QAccessible::registerAccessibleInterface(iface);
    childToId.insert(logicalIndex, QAccessible::uniqueId(iface));
    return iface;
}

void* AccessibleInstanceView::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::TableInterface)
        return static_cast<QAccessibleTableInterface*>(this);
    return 0;
}

void AccessibleInstanceView::modelChange(QAccessibleTableModelChangeEvent* event)
{
    // if there is no cache yet, we don't update anything
    if (childToId.isEmpty())
        return;

    switch (event->modelChangeType()) {
        case QAccessibleTableModelChangeEvent::ModelReset:
            for (QAccessible::Id id : childToId)
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
                QAccessibleInterface* iface = QAccessible::accessibleInterface(id);
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
                QAccessibleInterface* iface = QAccessible::accessibleInterface(id);
                Q_ASSERT(iface);
                if (iface->role() == QAccessible::Cell || iface->role() == QAccessible::ListItem) {
                    Q_ASSERT(iface->tableCellInterface());
                    AccessibleInstanceViewItem* cell = static_cast<AccessibleInstanceViewItem*>(iface->tableCellInterface());
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

AccessibleInstanceViewItem::AccessibleInstanceViewItem(QAbstractItemView* view_, const QModelIndex& index_) : view(view_), m_index(index_)
{
    if (Q_UNLIKELY(!index_.isValid()))
        qWarning() << "AccessibleInstanceViewItem::AccessibleInstanceViewItem with invalid index:" << index_;
}

void* AccessibleInstanceViewItem::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::TableCellInterface)
        return static_cast<QAccessibleTableCellInterface*>(this);
    if (t == QAccessible::ActionInterface)
        return static_cast<QAccessibleActionInterface*>(this);
    return 0;
}

int AccessibleInstanceViewItem::columnExtent() const
{
    return 1;
}
int AccessibleInstanceViewItem::rowExtent() const
{
    return 1;
}

QList<QAccessibleInterface*> AccessibleInstanceViewItem::rowHeaderCells() const
{
    return {};
}

QList<QAccessibleInterface*> AccessibleInstanceViewItem::columnHeaderCells() const
{
    return {};
}

int AccessibleInstanceViewItem::columnIndex() const
{
    if (!isValid()) {
        return -1;
    }

    return m_index.column();
}

int AccessibleInstanceViewItem::rowIndex() const
{
    if (!isValid()) {
        return -1;
    }

    return m_index.row();
}

bool AccessibleInstanceViewItem::isSelected() const
{
    if (!isValid()) {
        return false;
    }

    return view->selectionModel()->isSelected(m_index);
}

QStringList AccessibleInstanceViewItem::actionNames() const
{
    QStringList names;
    names << toggleAction();
    return names;
}

void AccessibleInstanceViewItem::doAction(const QString& actionName)
{
    if (actionName == toggleAction()) {
        if (isSelected()) {
            unselectCell();
        } else {
            selectCell();
        }
    }
}

QStringList AccessibleInstanceViewItem::keyBindingsForAction(const QString&) const
{
    return QStringList();
}

void AccessibleInstanceViewItem::selectCell()
{
    if (!isValid()) {
        return;
    }
    QAbstractItemView::SelectionMode selectionMode = view->selectionMode();
    if (selectionMode == QAbstractItemView::NoSelection) {
        return;
    }

    Q_ASSERT(table());
    QAccessibleTableInterface* cellTable = table()->tableInterface();

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

void AccessibleInstanceViewItem::unselectCell()
{
    if (!isValid())
        return;
    QAbstractItemView::SelectionMode selectionMode = view->selectionMode();
    if (selectionMode == QAbstractItemView::NoSelection)
        return;

    QAccessibleTableInterface* cellTable = table()->tableInterface();

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

    // If the mode is not MultiSelection or ExtendedSelection and only
    // one cell is selected it cannot be unselected by the user
    if ((selectionMode != QAbstractItemView::MultiSelection) && (selectionMode != QAbstractItemView::ExtendedSelection) &&
        (view->selectionModel()->selectedIndexes().count() <= 1))
        return;

    view->selectionModel()->select(m_index, QItemSelectionModel::Deselect);
}

QAccessibleInterface* AccessibleInstanceViewItem::table() const
{
    return QAccessible::queryAccessibleInterface(view);
}

QAccessible::Role AccessibleInstanceViewItem::role() const
{
    return QAccessible::ListItem;
}

QAccessible::State AccessibleInstanceViewItem::state() const
{
    QAccessible::State st;
    if (!isValid())
        return st;

    QRect globalRect = view->rect();
    globalRect.translate(view->mapToGlobal(QPoint(0, 0)));
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

QRect AccessibleInstanceViewItem::rect() const
{
    QRect r;
    if (!isValid())
        return r;
    r = view->visualRect(m_index);

    if (!r.isNull()) {
        r.translate(view->viewport()->mapTo(view, QPoint(0, 0)));
        r.translate(view->mapToGlobal(QPoint(0, 0)));
    }
    return r;
}

QString AccessibleInstanceViewItem::text(QAccessible::Text t) const
{
    QString value;
    if (!isValid())
        return value;
    QAbstractItemModel* model = view->model();
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

void AccessibleInstanceViewItem::setText(QAccessible::Text /*t*/, const QString& text)
{
    if (!isValid() || !(m_index.flags() & Qt::ItemIsEditable))
        return;
    view->model()->setData(m_index, text);
}

bool AccessibleInstanceViewItem::isValid() const
{
    return view && view->model() && m_index.isValid();
}

QAccessibleInterface* AccessibleInstanceViewItem::parent() const
{
    return QAccessible::queryAccessibleInterface(view);
}

QAccessibleInterface* AccessibleInstanceViewItem::child(int) const
{
    return 0;
}

#endif /* !QT_NO_ACCESSIBILITY */
