#include "ProjectItem.h"

#include "Common.h"

#include <QIcon>
#include <QPainter>

ProjectItemDelegate::ProjectItemDelegate(QWidget* parent) : QStyledItemDelegate(parent) {}

void ProjectItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    painter->save();

    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    auto rect = opt.rect;

    if (opt.state & QStyle::State_Selected) {
        painter->fillRect(rect, opt.palette.highlight());
        painter->setPen(opt.palette.highlightedText().color());
    } else if (opt.state & QStyle::State_MouseOver) {
        painter->fillRect(rect, opt.palette.window());
    }

    // The default icon size will be a square (and height is usually the lower value).
    auto icon_width = rect.height(), icon_height = rect.height();
    int icon_x_margin = (rect.height() - icon_width) / 2;
    int icon_y_margin = (rect.height() - icon_height) / 2;

    if (!opt.icon.isNull()) {  // Icon painting
        {
            auto icon_size = opt.decorationSize;
            icon_width = icon_size.width();
            icon_height = icon_size.height();

            icon_x_margin = (rect.height() - icon_width) / 2;
            icon_y_margin = (rect.height() - icon_height) / 2;
        }

        // Centralize icon with a margin to separate from the other elements
        int x = rect.x() + icon_x_margin;
        int y = rect.y() + icon_y_margin;

        // Prevent 'scaling null pixmap' warnings
        if (icon_width > 0 && icon_height > 0)
            opt.icon.paint(painter, x, y, icon_width, icon_height);
    }

    // Change the rect so that funther painting is easier
    auto remaining_width = rect.width() - icon_width - 2 * icon_x_margin;
    rect.setRect(rect.x() + icon_width + 2 * icon_x_margin, rect.y(), remaining_width, rect.height());

    {  // Title painting
        auto title = index.data(UserDataTypes::TITLE).toString();

        painter->save();

        auto font = opt.font;
        if (index.data(UserDataTypes::SELECTED).toBool()) {
            // Set nice font
            font.setBold(true);
            font.setUnderline(true);
        }

        font.setPointSize(font.pointSize() + 2);
        painter->setFont(font);

        // On the top, aligned to the left after the icon
        painter->drawText(rect.x(), rect.y() + QFontMetrics(font).height(), title);

        painter->restore();
    }

    {  // Description painting
        auto description = index.data(UserDataTypes::DESCRIPTION).toString();

        QTextLayout text_layout(description, opt.font);

        qreal height = 0;
        auto cut_text = viewItemTextLayout(text_layout, remaining_width, height);

        // Get first line unconditionally
        description = cut_text.first().second;
        // Get second line, elided if needed
        if (cut_text.size() > 1) {
            if (cut_text.size() > 2)
                description += opt.fontMetrics.elidedText(cut_text.at(1).second, opt.textElideMode, cut_text.at(1).first);
            else
                description += cut_text.at(1).second;
        }

        // On the bottom, aligned to the left after the icon, and featuring at most two lines of text (with some margin space to spare)
        painter->drawText(rect.x(), rect.y() + rect.height() - 2.0 * opt.fontMetrics.height(), remaining_width,
                          4 * opt.fontMetrics.height(), Qt::TextWordWrap, description);
    }

    painter->restore();
}
