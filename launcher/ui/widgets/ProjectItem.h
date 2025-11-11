#pragma once

#include <QStyledItemDelegate>

/* Custom data types for our custom list models :) */
enum UserDataTypes {
    TITLE = 257,        // QString
    DESCRIPTION = 258,  // QString
    INSTALLED = 259     // bool
};

/** This is an item delegate composed of:
 *  - An Icon on the left
 *  - A title
 *  - A description
 * */
class ProjectItemDelegate final : public QStyledItemDelegate {
    Q_OBJECT

   public:
    ProjectItemDelegate(QWidget* parent);

    void paint(QPainter*, const QStyleOptionViewItem&, const QModelIndex&) const override;

    bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;

   signals:
    void checkboxClicked(const QModelIndex& index);

   private:
    QStyleOptionViewItem makeCheckboxStyleOption(const QStyleOptionViewItem& opt, const QStyle* style) const;
};
