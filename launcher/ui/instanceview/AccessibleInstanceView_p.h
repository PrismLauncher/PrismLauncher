#pragma once

#include <QtGui/qaccessible.h>
#include <QAbstractItemView>
#include <QAccessibleWidget>
#include "QtCore/qpointer.h"
#ifndef QT_NO_ACCESSIBILITY
#include "InstanceView.h"
// #include <QHeaderView>

class QAccessibleTableCell;
class QAccessibleTableHeaderCell;

class AccessibleInstanceView : public QAccessibleTableInterface, public QAccessibleObject {
   public:
    explicit AccessibleInstanceView(QWidget* w);
    bool isValid() const override;

    QAccessible::Role role() const override;
    QAccessible::State state() const override;
    QString text(QAccessible::Text t) const override;
    QRect rect() const override;

    QAccessibleInterface* childAt(int x, int y) const override;
    int childCount() const override;
    int indexOfChild(const QAccessibleInterface*) const override;

    QAccessibleInterface* parent() const override;
    QAccessibleInterface* child(int index) const override;

    void* interface_cast(QAccessible::InterfaceType t) override;

    // table interface
    QAccessibleInterface* cellAt(int row, int column) const override;
    QAccessibleInterface* caption() const override;
    QAccessibleInterface* summary() const override;
    QString columnDescription(int column) const override;
    QString rowDescription(int row) const override;
    int columnCount() const override;
    int rowCount() const override;

    // selection
    int selectedCellCount() const override;
    int selectedColumnCount() const override;
    int selectedRowCount() const override;
    QList<QAccessibleInterface*> selectedCells() const override;
    QList<int> selectedColumns() const override;
    QList<int> selectedRows() const override;
    bool isColumnSelected(int column) const override;
    bool isRowSelected(int row) const override;
    bool selectRow(int row) override;
    bool selectColumn(int column) override;
    bool unselectRow(int row) override;
    bool unselectColumn(int column) override;

    QAbstractItemView* view() const;

    void modelChange(QAccessibleTableModelChangeEvent* event) override;

   protected:
    // maybe vector
    using ChildCache = QHash<int, QAccessible::Id>;
    mutable ChildCache childToId;

    virtual ~AccessibleInstanceView();

   private:
    inline int logicalIndex(const QModelIndex& index) const;
};

class AccessibleInstanceViewItem : public QAccessibleInterface, public QAccessibleTableCellInterface, public QAccessibleActionInterface {
   public:
    AccessibleInstanceViewItem(QAbstractItemView* view, const QModelIndex& m_index);

    void* interface_cast(QAccessible::InterfaceType t) override;
    QObject* object() const override { return nullptr; }
    QAccessible::Role role() const override;
    QAccessible::State state() const override;
    QRect rect() const override;
    bool isValid() const override;

    QAccessibleInterface* childAt(int, int) const override { return nullptr; }
    int childCount() const override { return 0; }
    int indexOfChild(const QAccessibleInterface*) const override { return -1; }

    QString text(QAccessible::Text t) const override;
    void setText(QAccessible::Text t, const QString& text) override;

    QAccessibleInterface* parent() const override;
    QAccessibleInterface* child(int) const override;

    // cell interface
    int columnExtent() const override;
    QList<QAccessibleInterface*> columnHeaderCells() const override;
    int columnIndex() const override;
    int rowExtent() const override;
    QList<QAccessibleInterface*> rowHeaderCells() const override;
    int rowIndex() const override;
    bool isSelected() const override;
    QAccessibleInterface* table() const override;

    // action interface
    QStringList actionNames() const override;
    void doAction(const QString& actionName) override;
    QStringList keyBindingsForAction(const QString& actionName) const override;

   private:
    QPointer<QAbstractItemView> view;
    QPersistentModelIndex m_index;

    void selectCell();
    void unselectCell();

    friend class AccessibleInstanceView;
};
#endif /* !QT_NO_ACCESSIBILITY */
