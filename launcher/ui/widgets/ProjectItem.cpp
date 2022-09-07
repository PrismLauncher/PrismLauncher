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

    auto& rect = opt.rect;
    auto icon_width = rect.height(), icon_height = rect.height();
    auto remaining_width = rect.width() - icon_width;

    if (opt.state & QStyle::State_Selected) {
        painter->fillRect(rect, opt.palette.highlight());
        painter->setPen(opt.palette.highlightedText().color());
    } else if (opt.state & QStyle::State_MouseOver) {
        painter->fillRect(rect, opt.palette.window());
    }

    {  // Icon painting
        // Square-sized, occupying the left portion
        opt.icon.paint(painter, rect.x(), rect.y(), icon_width, icon_height);
    }

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
        painter->drawText(rect.x() + icon_width, rect.y() + QFontMetrics(font).height(), title);

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
        painter->drawText(rect.x() + icon_width, rect.y() + rect.height() - 2.2 * opt.fontMetrics.height(), remaining_width,
                          2 * opt.fontMetrics.height(), Qt::TextWordWrap, description);
    }

    painter->restore();
}
