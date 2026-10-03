#pragma once

#include <QIcon>
#include <QList>
#include <QObject>
#include <QSize>
#include <QStyledItemDelegate>

class QModelIndex;
class QPainter;
class QStyleOptionViewItem;

/// Item delegate allowing DecorationRole to be provided as a QList<MultiDecorationItemDelegate::Icon>.
class MultiDecorationItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
   public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override;

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    struct Icon {
        QIcon icon;
        QSize size;
    };
};

Q_DECLARE_METATYPE(QList<MultiDecorationItemDelegate::Icon>);
