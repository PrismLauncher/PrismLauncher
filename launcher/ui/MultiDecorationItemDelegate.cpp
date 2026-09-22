#include "MultiDecorationItemDelegate.h"

namespace {
auto iconsAt(const QModelIndex& index)
{
    return index.data(Qt::DecorationRole).value<QList<MultiDecorationItemDelegate::Icon>>();
}

int iconSpacing(const QStyleOptionViewItem& opt) {
    QStyle* style = opt.widget != nullptr ? opt.widget->style() : QApplication::style();
    return (style->pixelMetric(QStyle::PM_FocusFrameHMargin, &opt, opt.widget) + 1) * 2;

}
}  // namespace

void MultiDecorationItemDelegate::initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const
{
    QStyledItemDelegate::initStyleOption(option, index);

    const int spacing = iconSpacing(*option);

    QList<Icon> icons = iconsAt(index);
    if (icons.isEmpty()) {
        return;
    }

    int width = 0;
    for (const auto& [icon, size] : icons) {
        if (width != 0) {
            width += spacing;
        }
        width += size.width();
    }

    // HACK: offset the drawing of the text so we can do our own custom drawing
    option->decorationSize = { width, 1 };
}

void MultiDecorationItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    const int spacing = iconSpacing(option);

    int x = 0;
    for (const auto& [icon, size] : iconsAt(index)) {
        if (x != 0) {
            x += spacing;
        }

        QSize actualSize = icon.actualSize(size);

        int y = (option.rect.height() - actualSize.height()) / 2;

        const QIcon::Mode mode = (option.state & QStyle::State_Selected) != 0 ? QIcon::Selected : QIcon::Normal;
        const QIcon::State state = (option.state & QStyle::State_Open) != 0 ? QIcon::On : QIcon::Off;

        icon.paint(painter, option.rect.x() + x, option.rect.y() + y, actualSize.width(), actualSize.height(), Qt::AlignLeft | Qt::AlignTop,
                   mode, state);
        x += size.width();
    }
}
