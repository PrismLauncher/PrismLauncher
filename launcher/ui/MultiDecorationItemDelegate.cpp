#include <QApplication>
#include <QIcon>
#include <QModelIndex>
#include <QPainter>
#include <QStyle>
#include <QStyleOption>
#include <Qt>

#include "MultiDecorationItemDelegate.h"
#include "ui/themes/FreshTheme.h"

namespace {
auto iconsAt(const QModelIndex& index)
{
    return index.data(Qt::DecorationRole).value<QList<MultiDecorationItemDelegate::Icon>>();
}

// Fresh: draws the icon clipped to a rounded square (antialiased, unlike a painter clip path)
void paintRoundedIcon(QPainter* painter, const QIcon& icon, const QRect& target, QIcon::Mode mode, QIcon::State state)
{
    const qreal ratio = painter->device()->devicePixelRatioF();
    const QPixmap source = icon.pixmap(target.size(), ratio, mode, state);
    // paint in device pixels: a brush would tile a high-dpi texture instead of covering the rect
    QPixmap texture = source;
    texture.setDevicePixelRatio(1);

    QPixmap rounded(source.size());
    rounded.fill(Qt::transparent);
    {
        QPainter roundedPainter(&rounded);
        roundedPainter.setRenderHint(QPainter::Antialiasing);
        roundedPainter.setPen(Qt::NoPen);
        roundedPainter.setBrush(texture);
        const qreal radius = qMin(source.width(), source.height()) * 0.22;
        roundedPainter.drawRoundedRect(QRectF(rounded.rect()), radius, radius);
    }
    rounded.setDevicePixelRatio(source.devicePixelRatio());
    painter->drawPixmap(target.topLeft(), rounded);
}

int iconSpacing(const QStyleOptionViewItem& opt)
{
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
        option->features &= ~QStyleOptionViewItem::HasDecoration;
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
    const bool rounded = FreshTheme::isActive();

    // start where the style reserved room for the decoration, so stylesheet padding is respected
    QStyleOptionViewItem decorationOption = option;
    initStyleOption(&decorationOption, index);
    QStyle* style = option.widget != nullptr ? option.widget->style() : QApplication::style();
    const int left = style->subElementRect(QStyle::SE_ItemViewItemDecoration, &decorationOption, option.widget).x();

    int x = 0;
    for (const auto& [icon, size] : iconsAt(index)) {
        if (x != 0) {
            x += spacing;
        }

        QSize actualSize = icon.actualSize(size);

        int y = (option.rect.height() - actualSize.height()) / 2;

        const QIcon::Mode mode = (option.state & QStyle::State_Selected) != 0 ? QIcon::Selected : QIcon::Normal;
        const QIcon::State state = (option.state & QStyle::State_Open) != 0 ? QIcon::On : QIcon::Off;

        const QRect target(left + x, option.rect.y() + y, actualSize.width(), actualSize.height());
        if (rounded && actualSize.width() >= 24) {
            paintRoundedIcon(painter, icon, target, mode, state);
        } else {
            icon.paint(painter, target, Qt::AlignLeft | Qt::AlignTop, mode, state);
        }
        x += size.width();
    }
}
