// SPDX-License-Identifier: GPL-3.0-only
#include "BlocksStyle.h"

#include <QAbstractButton>
#include <QAbstractSpinBox>
#include <QComboBox>
#include <QLinearGradient>
#include <QPainter>
#include <QScrollBar>
#include <QStyleOption>
#include <QTabBar>

#include <utility>

namespace {

// Pixel glyphs: one string per row, '#' marks a lit pixel
const char* const s_checkMark[] = {
    "......#",  //
    ".....##",  //
    "#...##.",  //
    "##.##..",  //
    ".###...",  //
    "..#....",  //
};
const char* const s_dash[] = {
    "#####",  //
    "#####",  //
};
const char* const s_radioFill[] = {
    "..####..",  //
    ".######.",  //
    "########",  //
    "########",  //
    "########",  //
    "########",  //
    ".######.",  //
    "..####..",  //
};
const char* const s_radioRing[] = {
    "..####..",  //
    ".#....#.",  //
    "#......#",  //
    "#......#",  //
    "#......#",  //
    "#......#",  //
    ".#....#.",  //
    "..####..",  //
};
const char* const s_radioDot[] = {
    ".##.",  //
    "####",  //
    "####",  //
    ".##.",  //
};
const char* const s_arrowDown[] = {
    "#######",  //
    ".#####.",  //
    "..###..",  //
    "...#...",  //
};
const char* const s_arrowUp[] = {
    "...#...",  //
    "..###..",  //
    ".#####.",  //
    "#######",  //
};
const char* const s_arrowRight[] = {
    "#...",  //
    "##..",  //
    "###.",  //
    "####",  //
    "###.",  //
    "##..",  //
    "#...",  //
};
const char* const s_arrowLeft[] = {
    "...#",  //
    "..##",  //
    ".###",  //
    "####",  //
    ".###",  //
    "..##",  //
    "...#",  //
};

void drawFrameEdges(QPainter* painter, const QRect& rect, int width, const QColor& color)
{
    painter->fillRect(QRect(rect.left(), rect.top(), rect.width(), width), color);
    painter->fillRect(QRect(rect.left(), rect.bottom() - width + 1, rect.width(), width), color);
    painter->fillRect(QRect(rect.left(), rect.top() + width, width, rect.height() - 2 * width), color);
    painter->fillRect(QRect(rect.right() - width + 1, rect.top() + width, width, rect.height() - 2 * width), color);
}

}  // namespace

BlocksStyle::BlocksStyle(QStyle* base, const Colors& colors) : HintOverrideProxyStyle(base), m_colors(colors) {}

void BlocksStyle::polish(QWidget* widget)
{
    HintOverrideProxyStyle::polish(widget);
    if (qobject_cast<QAbstractButton*>(widget) || qobject_cast<QComboBox*>(widget) || qobject_cast<QAbstractSpinBox*>(widget) ||
        qobject_cast<QScrollBar*>(widget) || qobject_cast<QTabBar*>(widget)) {
        widget->setAttribute(Qt::WA_Hover, true);
    }
}

void BlocksStyle::drawRaised(QPainter* painter,
                             QRect rect,
                             const QColor& face,
                             const QColor& light,
                             const QColor& dark,
                             const QColor& outline) const
{
    if (outline.isValid()) {
        drawFrameEdges(painter, rect, 1, outline);
        rect.adjust(1, 1, -1, -1);
    }
    if (face.alpha() > 0) {
        painter->fillRect(rect, face);
    }
    const int bevel = qMin(m_unit, qMin(rect.width(), rect.height()) / 3);
    if (bevel <= 0) {
        return;
    }
    painter->fillRect(QRect(rect.left(), rect.top(), rect.width(), bevel), light);
    painter->fillRect(QRect(rect.left(), rect.top(), bevel, rect.height()), light);
    painter->fillRect(QRect(rect.left() + bevel, rect.bottom() - bevel + 1, rect.width() - bevel, bevel), dark);
    painter->fillRect(QRect(rect.right() - bevel + 1, rect.top() + bevel, bevel, rect.height() - bevel), dark);
}

void BlocksStyle::drawSunken(QPainter* painter, QRect rect, const QColor& face, const QColor& dark, const QColor& light) const
{
    drawRaised(painter, rect, face, dark, light, QColor());
}

void BlocksStyle::drawButtonBevel(QPainter* painter, const QRect& rect, QStyle::State state, bool primary) const
{
    const bool enabled = state & State_Enabled;
    const bool down = state & (State_Sunken | State_On);
    const bool keyboardFocus = (state & State_HasFocus) && (state & State_KeyboardFocusChange);
    const bool hot = enabled && ((state & State_MouseOver) || keyboardFocus);

    const QColor base = primary ? m_colors.primary : m_colors.button;
    QColor face = base;
    QColor light = primary ? m_colors.primaryLight : m_colors.buttonLight;
    QColor dark = primary ? m_colors.primaryDark : m_colors.buttonDark;
    if (!enabled) {
        face = m_colors.buttonDisabled;
        light = face.lighter(120);
        dark = face.darker(140);
    } else if (down) {
        face = base.darker(118);
        std::swap(light, dark);
    } else if (hot) {
        face = base.lighter(112);
    }
    // Minecraft marks the hovered or focused button with a white edge instead of the black one
    drawRaised(painter, rect, face, light, dark, hot ? m_colors.buttonOutlineActive : m_colors.outline);
}

void BlocksStyle::drawField(QPainter* painter, const QRect& rect, QStyle::State state, const QColor& fill) const
{
    QColor border = m_colors.fieldBorder;
    if (!(state & State_Enabled)) {
        border = m_colors.fieldBorder.darker(160);
    } else if (state & State_HasFocus) {
        border = m_colors.fieldBorderFocus;
    }
    painter->fillRect(rect.adjusted(1, 1, -1, -1), fill);
    drawFrameEdges(painter, rect, 1, border);
}

void BlocksStyle::drawGlyph(QPainter* painter, const QRect& rect, const char* const* rows, int columns, int lines, const QColor& color)
    const
{
    const int pixel = qMax(1, qMin(rect.width() / columns, rect.height() / lines));
    const int left = rect.left() + (rect.width() - columns * pixel) / 2;
    const int top = rect.top() + (rect.height() - lines * pixel) / 2;
    for (int y = 0; y < lines; ++y) {
        for (int x = 0; x < columns; ++x) {
            if (rows[y][x] == '#') {
                painter->fillRect(QRect(left + x * pixel, top + y * pixel, pixel, pixel), color);
            }
        }
    }
}

void BlocksStyle::drawArrow(QPainter* painter, const QRect& rect, Qt::ArrowType direction, const QColor& color) const
{
    // Arrows are drawn with chunky pixels when there is room, and never larger than two pixels per dot
    const bool vertical = direction == Qt::UpArrow || direction == Qt::DownArrow;
    const int columns = vertical ? 7 : 4;
    const int lines = vertical ? 4 : 7;
    const int pixel = qBound(1, qMin(rect.width() / (columns + 2), rect.height() / (lines + 2)), 2);
    const QRect area(rect.center().x() - columns * pixel / 2, rect.center().y() - lines * pixel / 2, columns * pixel, lines * pixel);
    switch (direction) {
        case Qt::UpArrow:
            drawGlyph(painter, area, s_arrowUp, columns, lines, color);
            break;
        case Qt::DownArrow:
            drawGlyph(painter, area, s_arrowDown, columns, lines, color);
            break;
        case Qt::LeftArrow:
            drawGlyph(painter, area, s_arrowLeft, columns, lines, color);
            break;
        default:
            drawGlyph(painter, area, s_arrowRight, columns, lines, color);
            break;
    }
}

void BlocksStyle::drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
{
    const QRect& rect = option->rect;
    const bool enabled = option->state & State_Enabled;

    switch (element) {
        case PE_PanelButtonCommand: {
            const auto* button = qstyleoption_cast<const QStyleOptionButton*>(option);
            const bool flat = button && (button->features & QStyleOptionButton::Flat);
            if (flat && !(option->state & (State_Sunken | State_On | State_MouseOver))) {
                return;
            }
            drawButtonBevel(painter, rect, option->state);
            return;
        }
        case PE_PanelButtonBevel:
        case PE_PanelButtonTool:
            drawButtonBevel(painter, rect, option->state);
            return;
        case PE_FrameDefaultButton:
        case PE_FrameButtonBevel:
        case PE_FrameButtonTool:
            // Part of the bevel already
            return;

        case PE_PanelLineEdit: {
            const auto* frame = qstyleoption_cast<const QStyleOptionFrame*>(option);
            if (frame && frame->lineWidth > 0) {
                drawField(painter, rect, option->state, option->palette.color(QPalette::Base));
            } else {
                painter->fillRect(rect, option->palette.brush(QPalette::Base));
            }
            return;
        }
        case PE_FrameLineEdit:
            drawFrameEdges(painter, rect, 1, (option->state & State_HasFocus) ? m_colors.fieldBorderFocus : m_colors.fieldBorder);
            return;

        case PE_Frame:
        case PE_FrameWindow:
            // Views and text areas sit in an inventory-style recess
            painter->fillRect(QRect(rect.left(), rect.top(), rect.width(), m_unit), m_colors.slotDark);
            painter->fillRect(QRect(rect.left(), rect.top(), m_unit, rect.height()), m_colors.slotDark);
            painter->fillRect(QRect(rect.left() + m_unit, rect.bottom() - m_unit + 1, rect.width() - m_unit, m_unit), m_colors.slotLight);
            painter->fillRect(QRect(rect.right() - m_unit + 1, rect.top() + m_unit, m_unit, rect.height() - m_unit), m_colors.slotLight);
            return;

        case PE_FrameGroupBox: {
            const auto* frame = qstyleoption_cast<const QStyleOptionFrame*>(option);
            if (frame && (frame->features & QStyleOptionFrame::Flat)) {
                painter->fillRect(QRect(rect.left(), rect.top(), rect.width(), 1), m_colors.panelDark);
                painter->fillRect(QRect(rect.left(), rect.top() + 1, rect.width(), 1), m_colors.panelLight);
                return;
            }
            drawFrameEdges(painter, rect.adjusted(0, 0, -1, -1), 1, m_colors.panelDark);
            drawFrameEdges(painter, rect.adjusted(1, 1, 0, 0), 1, m_colors.panelLight);
            return;
        }

        case PE_FrameTabWidget:
            drawRaised(painter, rect, m_colors.panel, m_colors.panelLight, m_colors.panelDark, m_colors.outline);
            return;
        case PE_FrameTabBarBase:
            return;

        case PE_PanelMenu:
        case PE_FrameMenu: {
            // Menus use the view colours, like Minecraft's dark chat and tooltip boxes
            const QColor face = option->palette.color(QPalette::Base).lighter(160);
            if (element == PE_PanelMenu) {
                painter->fillRect(rect, face);
            }
            drawFrameEdges(painter, rect, 1, m_colors.outline);
            drawFrameEdges(painter, rect.adjusted(1, 1, -1, -1), 1, face.lighter(170));
            return;
        }

        case PE_PanelTipLabel: {
            painter->fillRect(rect, m_colors.tooltip);
            QLinearGradient border(rect.topLeft(), rect.bottomLeft());
            border.setColorAt(0, m_colors.tooltipBorderTop);
            border.setColorAt(1, m_colors.tooltipBorderBottom);
            const QRect inner = rect.adjusted(1, 1, -1, -1);
            painter->fillRect(QRect(inner.left(), inner.top(), inner.width(), 1), m_colors.tooltipBorderTop);
            painter->fillRect(QRect(inner.left(), inner.bottom(), inner.width(), 1), m_colors.tooltipBorderBottom);
            painter->fillRect(QRect(inner.left(), inner.top(), 1, inner.height()), border);
            painter->fillRect(QRect(inner.right(), inner.top(), 1, inner.height()), border);
            return;
        }

        case PE_IndicatorCheckBox:
        case PE_IndicatorItemViewItemCheck: {
            const QRect box(rect.x(), rect.y() + (rect.height() - 18) / 2, qMin(rect.width(), 18), qMin(rect.height(), 18));
            QStyle::State state = option->state;
            if (state & State_MouseOver) {
                state |= State_HasFocus;  // a white edge marks the box under the pointer
            }
            drawField(painter, box, state, m_colors.field);
            const QColor mark = enabled ? QColor(Qt::white) : m_colors.fieldBorder;
            if (option->state & State_On) {
                drawGlyph(painter, box.adjusted(2, 2, -2, -2), s_checkMark, 7, 6, mark);
            } else if (option->state & State_NoChange) {
                drawGlyph(painter, box.adjusted(3, 3, -3, -3), s_dash, 5, 2, mark);
            }
            return;
        }
        case PE_IndicatorRadioButton: {
            const QRect box(rect.x(), rect.y() + (rect.height() - 18) / 2, qMin(rect.width(), 18), qMin(rect.height(), 18));
            const QRect glyph = box.adjusted(1, 1, -1, -1);
            const bool hot = enabled && (option->state & (State_MouseOver | State_HasFocus));
            drawGlyph(painter, glyph, s_radioFill, 8, 8, m_colors.field);
            drawGlyph(painter, glyph, s_radioRing, 8, 8, hot ? m_colors.fieldBorderFocus : m_colors.fieldBorder);
            if (option->state & State_On) {
                drawGlyph(painter, glyph.adjusted(4, 4, -4, -4), s_radioDot, 4, 4, enabled ? QColor(Qt::white) : m_colors.fieldBorder);
            }
            return;
        }

        case PE_IndicatorArrowUp:
        case PE_IndicatorArrowDown:
        case PE_IndicatorArrowLeft:
        case PE_IndicatorArrowRight:
        case PE_IndicatorSpinUp:
        case PE_IndicatorSpinDown:
        case PE_IndicatorSpinPlus:
        case PE_IndicatorSpinMinus: {
            Qt::ArrowType direction = Qt::RightArrow;
            if (element == PE_IndicatorArrowUp || element == PE_IndicatorSpinUp || element == PE_IndicatorSpinPlus) {
                direction = Qt::UpArrow;
            } else if (element == PE_IndicatorArrowDown || element == PE_IndicatorSpinDown || element == PE_IndicatorSpinMinus) {
                direction = Qt::DownArrow;
            } else if (element == PE_IndicatorArrowLeft) {
                direction = Qt::LeftArrow;
            }
            drawArrow(painter, rect, direction,
                      option->palette.color(enabled ? QPalette::Active : QPalette::Disabled, QPalette::WindowText));
            return;
        }
        case PE_IndicatorHeaderArrow: {
            const auto* header = qstyleoption_cast<const QStyleOptionHeader*>(option);
            if (header && header->sortIndicator != QStyleOptionHeader::None) {
                drawArrow(painter, rect, header->sortIndicator == QStyleOptionHeader::SortUp ? Qt::UpArrow : Qt::DownArrow,
                          option->palette.color(QPalette::ButtonText));
            }
            return;
        }
        case PE_IndicatorBranch: {
            if (option->state & State_Children) {
                drawArrow(painter, rect, (option->state & State_Open) ? Qt::DownArrow : Qt::RightArrow,
                          option->palette.color(QPalette::Text));
            }
            return;
        }

        case PE_IndicatorToolBarHandle: {
            const bool horizontal = option->state & State_Horizontal;
            const QPoint center = rect.center();
            for (int i = -2; i <= 2; ++i) {
                const QPoint dot =
                    horizontal ? QPoint(center.x() - 1, center.y() + i * 4 - 1) : QPoint(center.x() + i * 4 - 1, center.y() - 1);
                painter->fillRect(QRect(dot + QPoint(1, 1), QSize(2, 2)), m_colors.panelLight);
                painter->fillRect(QRect(dot, QSize(2, 2)), m_colors.panelDark);
            }
            return;
        }
        case PE_IndicatorToolBarSeparator: {
            const QPoint center = rect.center();
            if (option->state & State_Horizontal) {
                painter->fillRect(QRect(center.x() - 1, rect.top() + 3, 1, rect.height() - 6), m_colors.panelDark);
                painter->fillRect(QRect(center.x(), rect.top() + 3, 1, rect.height() - 6), m_colors.panelLight);
            } else {
                painter->fillRect(QRect(rect.left() + 3, center.y() - 1, rect.width() - 6, 1), m_colors.panelDark);
                painter->fillRect(QRect(rect.left() + 3, center.y(), rect.width() - 6, 1), m_colors.panelLight);
            }
            return;
        }

        case PE_PanelStatusBar:
            painter->fillRect(rect, m_colors.panel);
            painter->fillRect(QRect(rect.left(), rect.top(), rect.width(), 1), m_colors.panelDark);
            painter->fillRect(QRect(rect.left(), rect.top() + 1, rect.width(), 1), m_colors.panelLight);
            return;

        case PE_PanelScrollAreaCorner:
            painter->fillRect(rect, m_colors.xpTrack);
            return;

        case PE_PanelItemViewItem: {
            const auto* item = qstyleoption_cast<const QStyleOptionViewItem*>(option);
            if (!item || !(option->state & State_Selected)) {
                break;
            }
            const auto group = enabled ? QPalette::Normal : QPalette::Disabled;
            painter->fillRect(rect, option->palette.brush(group, QPalette::Highlight));
            // Outline the selected entry, as Minecraft does in its world and server lists
            const QColor edge = option->palette.color(group, QPalette::HighlightedText);
            const auto position = item->viewItemPosition;
            const bool first = position == QStyleOptionViewItem::Beginning || position == QStyleOptionViewItem::OnlyOne ||
                               position == QStyleOptionViewItem::Invalid;
            const bool last = position == QStyleOptionViewItem::End || position == QStyleOptionViewItem::OnlyOne ||
                              position == QStyleOptionViewItem::Invalid;
            painter->fillRect(QRect(rect.left(), rect.top(), rect.width(), 1), edge);
            painter->fillRect(QRect(rect.left(), rect.bottom(), rect.width(), 1), edge);
            if (first) {
                painter->fillRect(QRect(rect.left(), rect.top(), 1, rect.height()), edge);
            }
            if (last) {
                painter->fillRect(QRect(rect.right(), rect.top(), 1, rect.height()), edge);
            }
            return;
        }

        default:
            break;
    }
    HintOverrideProxyStyle::drawPrimitive(element, option, painter, widget);
}

void BlocksStyle::drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
{
    const QRect& rect = option->rect;

    switch (element) {
        case CE_ToolBar:
            drawRaised(painter, rect, m_colors.panel, m_colors.panelLight, m_colors.panelDark, QColor());
            return;

        case CE_Splitter: {
            painter->fillRect(rect, m_colors.panel);
            const QPoint center = rect.center();
            const bool horizontal = option->state & State_Horizontal;
            for (int i = -1; i <= 1; ++i) {
                const QPoint dot =
                    horizontal ? QPoint(center.x() - 1, center.y() + i * 5 - 1) : QPoint(center.x() + i * 5 - 1, center.y() - 1);
                painter->fillRect(QRect(dot, QSize(2, 2)), m_colors.panelDark);
            }
            return;
        }

        case CE_HeaderSection:
            drawRaised(painter, rect, m_colors.button, m_colors.buttonLight, m_colors.buttonDark, QColor());
            return;
        case CE_HeaderEmptyArea:
            painter->fillRect(rect, m_colors.button);
            return;

        case CE_TabBarTabShape: {
            const auto* tab = qstyleoption_cast<const QStyleOptionTab*>(option);
            if (!tab || (tab->shape != QTabBar::RoundedNorth && tab->shape != QTabBar::TriangularNorth)) {
                break;
            }
            const bool selected = option->state & State_Selected;
            QRect shape = rect;
            if (selected) {
                // Overlap the page frame so the active tab joins its page
                shape.adjust(0, 0, 0, proxy()->pixelMetric(PM_TabBarBaseOverlap, option, widget));
            } else {
                shape.adjust(0, m_unit * 2, 0, 0);
            }
            const QColor face = selected ? m_colors.panel : m_colors.panel.darker(118);
            painter->fillRect(QRect(shape.left(), shape.top(), shape.width(), 1), m_colors.outline);
            painter->fillRect(QRect(shape.left(), shape.top(), 1, shape.height()), m_colors.outline);
            painter->fillRect(QRect(shape.right(), shape.top(), 1, shape.height()), m_colors.outline);
            const QRect inner = shape.adjusted(1, 1, -1, selected ? 0 : -1);
            painter->fillRect(inner, face);
            painter->fillRect(QRect(inner.left(), inner.top(), inner.width(), m_unit), m_colors.panelLight);
            painter->fillRect(QRect(inner.left(), inner.top(), m_unit, inner.height()), m_colors.panelLight);
            painter->fillRect(QRect(inner.right() - m_unit + 1, inner.top() + m_unit, m_unit, inner.height() - m_unit), m_colors.panelDark);
            if (!selected) {
                painter->fillRect(QRect(shape.left(), shape.bottom(), shape.width(), 1), m_colors.outline);
            }
            return;
        }

        case CE_ProgressBarGroove:
            painter->fillRect(rect, m_colors.outline);
            painter->fillRect(rect.adjusted(1, 1, -1, -1), m_colors.xpTrack);
            return;

        case CE_ProgressBarContents: {
            const auto* bar = qstyleoption_cast<const QStyleOptionProgressBar*>(option);
            if (!bar || !(bar->state & State_Horizontal)) {
                break;
            }
            const QRect track = rect.adjusted(1, 1, -1, -1);
            if (track.width() <= 0 || track.height() <= 0) {
                return;
            }
            const bool busy = bar->minimum == 0 && bar->maximum == 0;
            int filled = track.width();
            if (!busy) {
                const qint64 range = qint64(bar->maximum) - bar->minimum;
                const qint64 done = qint64(bar->progress) - bar->minimum;
                filled = range > 0 ? int(track.width() * qBound<qint64>(0, done, range) / range) : 0;
            }
            QRect fill = track;
            fill.setWidth(filled);
            if (bar->invertedAppearance) {
                fill.moveRight(track.right());
            }
            if (busy) {
                // No known length: alternating blocks, like an empty XP bar waiting to fill
                for (int x = track.left(); x < track.right(); x += 16) {
                    painter->fillRect(QRect(x, track.top(), qMin(8, track.right() - x + 1), track.height()), m_colors.xpDark);
                }
                return;
            }
            painter->fillRect(fill, m_colors.xp);
            painter->fillRect(QRect(fill.left(), fill.top(), fill.width(), 1), m_colors.xp.lighter(135));
            // Notches every 10% of the track, as on the experience bar
            for (int i = 1; i < 10; ++i) {
                const int x = track.left() + track.width() * i / 10;
                painter->fillRect(QRect(x, track.top(), 1, track.height()), x <= fill.right() ? m_colors.xpDark : m_colors.outline);
            }
            return;
        }
        case CE_ProgressBarLabel: {
            const auto* bar = qstyleoption_cast<const QStyleOptionProgressBar*>(option);
            if (!bar || !bar->textVisible || bar->text.isEmpty()) {
                return;
            }
            const int flags = Qt::AlignCenter | Qt::TextSingleLine;
            painter->save();
            painter->setPen(m_colors.textShadow);
            painter->drawText(rect.translated(1, 1), flags, bar->text);
            painter->setPen(Qt::white);
            painter->drawText(rect, flags, bar->text);
            painter->restore();
            return;
        }

        default:
            break;
    }
    HintOverrideProxyStyle::drawControl(element, option, painter, widget);
}

void BlocksStyle::drawComplexControl(ComplexControl control,
                                     const QStyleOptionComplex* option,
                                     QPainter* painter,
                                     const QWidget* widget) const
{
    const bool enabled = option->state & State_Enabled;

    switch (control) {
        case CC_ToolButton: {
            const auto* toolButton = qstyleoption_cast<const QStyleOptionToolButton*>(option);
            if (!toolButton) {
                break;
            }
            const QRect button = proxy()->subControlRect(control, toolButton, SC_ToolButton, widget);
            const QRect menu = proxy()->subControlRect(control, toolButton, SC_ToolButtonMenu, widget);
            State buttonState = toolButton->state;
            buttonState.setFlag(State_Sunken, false);
            State menuState = buttonState;
            if (toolButton->state & State_Sunken) {
                if (toolButton->activeSubControls & SC_ToolButton) {
                    buttonState |= State_Sunken;
                }
                menuState |= State_Sunken;
            }
            const bool hasText = toolButton->toolButtonStyle != Qt::ToolButtonIconOnly && !toolButton->text.isEmpty();
            const bool active = toolButton->state & (State_Sunken | State_On | State_MouseOver);
            const bool primary = widget && widget->property("_prism_primary").toBool();
            // A button with a label is always a stone button: its light text needs the dark face behind it
            const bool bevel = hasText || active || !(toolButton->state & State_AutoRaise);
            if (bevel) {
                drawButtonBevel(painter, button, buttonState, primary);
                if (toolButton->subControls & SC_ToolButtonMenu) {
                    drawButtonBevel(painter, menu, menuState, primary);
                }
            }

            QStyleOptionToolButton label = *toolButton;
            label.state = buttonState;
            const int frame = proxy()->pixelMetric(PM_DefaultFrameWidth, toolButton, widget);
            label.rect = button.adjusted(frame, frame, -frame, -frame);
            if (!bevel) {
                // On the bare panel, draw any arrow or text in the panel's text colour
                label.palette.setColor(QPalette::ButtonText, label.palette.color(QPalette::WindowText));
            }
            painter->save();
            if (primary) {
                // The primary action reads like Minecraft's own big "Play" button
                QFont font = painter->font();
                font.setPixelSize(qMax(font.pixelSize(), 20));
                painter->setFont(font);
                label.font = font;
                label.fontMetrics = QFontMetrics(font);
            }
            proxy()->drawControl(CE_ToolButtonLabel, &label, painter, widget);
            painter->restore();

            const QColor arrow = toolButton->palette.color(enabled ? QPalette::Active : QPalette::Disabled,
                                                           bevel ? QPalette::ButtonText : QPalette::WindowText);
            if (toolButton->subControls & SC_ToolButtonMenu) {
                drawArrow(painter, menu, Qt::DownArrow, arrow);
            } else if (toolButton->features & QStyleOptionToolButton::HasMenu) {
                const QRect corner(button.right() - 12, button.bottom() - 9, 9, 6);
                drawArrow(painter, corner, Qt::DownArrow, arrow);
            }
            return;
        }

        case CC_ComboBox: {
            const auto* combo = qstyleoption_cast<const QStyleOptionComboBox*>(option);
            if (!combo) {
                break;
            }
            const QRect arrow = proxy()->subControlRect(control, combo, SC_ComboBoxArrow, widget);
            if (combo->editable) {
                drawField(painter, combo->rect, combo->state, combo->palette.color(QPalette::Base));
                State arrowState = combo->state & (State_Enabled | State_MouseOver);
                if ((combo->state & State_Sunken) && (combo->activeSubControls & SC_ComboBoxArrow)) {
                    arrowState |= State_Sunken;
                }
                drawButtonBevel(painter, arrow.adjusted(1, 1, -1, -1), arrowState);
            } else {
                drawButtonBevel(painter, combo->rect, combo->state);
            }
            drawArrow(painter, arrow, Qt::DownArrow,
                      combo->palette.color(enabled ? QPalette::Active : QPalette::Disabled, QPalette::ButtonText));
            return;
        }

        case CC_SpinBox: {
            const auto* spin = qstyleoption_cast<const QStyleOptionSpinBox*>(option);
            if (!spin) {
                break;
            }
            if (spin->frame && (spin->subControls & SC_SpinBoxFrame)) {
                drawField(painter, spin->rect, spin->state, spin->palette.color(QPalette::Base));
            } else {
                painter->fillRect(spin->rect, spin->palette.brush(QPalette::Base));
            }
            if (spin->buttonSymbols == QAbstractSpinBox::NoButtons) {
                return;
            }
            const auto drawStep = [&](SubControl subControl, QAbstractSpinBox::StepEnabledFlag flag, Qt::ArrowType direction) {
                const QRect stepRect = proxy()->subControlRect(control, spin, subControl, widget);
                State state = spin->stepEnabled.testFlag(flag) && enabled ? State_Enabled : State_None;
                if (spin->activeSubControls == subControl) {
                    state |= spin->state & (State_Sunken | State_MouseOver);
                }
                drawButtonBevel(painter,
                                stepRect.adjusted(0, subControl == SC_SpinBoxUp ? 1 : 0, -1, subControl == SC_SpinBoxDown ? -1 : 0), state);
                drawArrow(painter, stepRect, direction,
                          spin->palette.color((state & State_Enabled) ? QPalette::Active : QPalette::Disabled, QPalette::ButtonText));
            };
            drawStep(SC_SpinBoxUp, QAbstractSpinBox::StepUpEnabled, Qt::UpArrow);
            drawStep(SC_SpinBoxDown, QAbstractSpinBox::StepDownEnabled, Qt::DownArrow);
            return;
        }

        case CC_ScrollBar: {
            const auto* scrollBar = qstyleoption_cast<const QStyleOptionSlider*>(option);
            if (!scrollBar) {
                break;
            }
            painter->fillRect(proxy()->subControlRect(control, scrollBar, SC_ScrollBarGroove, widget), m_colors.xpTrack);
            if (scrollBar->subControls & SC_ScrollBarSlider) {
                State state = scrollBar->state & State_Enabled;
                if (scrollBar->activeSubControls & SC_ScrollBarSlider) {
                    state |= scrollBar->state & (State_Sunken | State_MouseOver);
                }
                const QRect slider = proxy()->subControlRect(control, scrollBar, SC_ScrollBarSlider, widget);
                drawButtonBevel(painter, slider.adjusted(1, 1, -1, -1), state);
            }
            return;
        }

        case CC_Slider: {
            const auto* slider = qstyleoption_cast<const QStyleOptionSlider*>(option);
            if (!slider) {
                break;
            }
            const QRect groove = proxy()->subControlRect(control, slider, SC_SliderGroove, widget);
            const QRect handle = proxy()->subControlRect(control, slider, SC_SliderHandle, widget);
            if (slider->subControls & SC_SliderGroove) {
                const QRect track = slider->orientation == Qt::Horizontal
                                        ? QRect(groove.left(), groove.center().y() - 3, groove.width(), 6)
                                        : QRect(groove.center().x() - 3, groove.top(), 6, groove.height());
                drawField(painter, track, slider->state & State_Enabled, m_colors.field);
            }
            if (slider->subControls & SC_SliderHandle) {
                State state = slider->state & (State_Enabled | State_HasFocus | State_KeyboardFocusChange);
                if (slider->activeSubControls & SC_SliderHandle) {
                    state |= slider->state & (State_Sunken | State_MouseOver);
                }
                drawButtonBevel(painter, handle, state);
            }
            return;
        }

        case CC_GroupBox: {
            const auto* groupBox = qstyleoption_cast<const QStyleOptionGroupBox*>(option);
            if (!groupBox) {
                break;
            }
            if (groupBox->subControls & SC_GroupBoxFrame) {
                QStyleOptionFrame frame;
                frame.QStyleOption::operator=(*groupBox);
                frame.features = groupBox->features;
                frame.lineWidth = groupBox->lineWidth;
                frame.midLineWidth = groupBox->midLineWidth;
                frame.rect = proxy()->subControlRect(control, groupBox, SC_GroupBoxFrame, widget);
                proxy()->drawPrimitive(PE_FrameGroupBox, &frame, painter, widget);
            }
            QStyleOptionGroupBox rest = *groupBox;
            rest.subControls &= ~SC_GroupBoxFrame;
            HintOverrideProxyStyle::drawComplexControl(control, &rest, painter, widget);
            return;
        }

        default:
            break;
    }
    HintOverrideProxyStyle::drawComplexControl(control, option, painter, widget);
}

void BlocksStyle::drawItemText(QPainter* painter,
                               const QRect& rect,
                               int flags,
                               const QPalette& palette,
                               bool enabled,
                               const QString& text,
                               QPalette::ColorRole textRole) const
{
    if (textRole == QPalette::ButtonText && enabled && !text.isEmpty()) {
        // Light text on a stone face gets a hard drop shadow, one pixel down and to the right
        painter->save();
        painter->setPen(m_colors.textShadow);
        painter->drawText(rect.translated(1, 1), flags, text);
        painter->restore();
    }
    HintOverrideProxyStyle::drawItemText(painter, rect, flags, palette, enabled, text, textRole);
}

QRect BlocksStyle::subControlRect(ComplexControl control,
                                  const QStyleOptionComplex* option,
                                  SubControl subControl,
                                  const QWidget* widget) const
{
    if (control == CC_ScrollBar) {
        if (const auto* scrollBar = qstyleoption_cast<const QStyleOptionSlider*>(option)) {
            // No arrow buttons: the whole bar is track, like Minecraft's scroll bars
            const QRect bar = scrollBar->rect;
            const bool horizontal = scrollBar->orientation == Qt::Horizontal;
            const int length = horizontal ? bar.width() : bar.height();
            int sliderLength = length;
            if (scrollBar->maximum != scrollBar->minimum) {
                const qint64 range = qint64(scrollBar->maximum) - scrollBar->minimum;
                sliderLength = int(qint64(scrollBar->pageStep) * length / (range + scrollBar->pageStep));
                sliderLength = qBound(qMin(proxy()->pixelMetric(PM_ScrollBarSliderMin, scrollBar, widget), length), sliderLength, length);
            }
            const int sliderStart = sliderPositionFromValue(scrollBar->minimum, scrollBar->maximum, scrollBar->sliderPosition,
                                                            length - sliderLength, scrollBar->upsideDown);
            QRect result;
            switch (subControl) {
                case SC_ScrollBarSubLine:
                case SC_ScrollBarAddLine:
                    return QRect();
                case SC_ScrollBarSubPage:
                    result = horizontal ? QRect(0, 0, sliderStart, bar.height()) : QRect(0, 0, bar.width(), sliderStart);
                    break;
                case SC_ScrollBarAddPage:
                    result = horizontal ? QRect(sliderStart + sliderLength, 0, length - sliderStart - sliderLength, bar.height())
                                        : QRect(0, sliderStart + sliderLength, bar.width(), length - sliderStart - sliderLength);
                    break;
                case SC_ScrollBarSlider:
                    result =
                        horizontal ? QRect(sliderStart, 0, sliderLength, bar.height()) : QRect(0, sliderStart, bar.width(), sliderLength);
                    break;
                case SC_ScrollBarGroove:
                    result = QRect(0, 0, bar.width(), bar.height());
                    break;
                default:
                    break;
            }
            return visualRect(scrollBar->direction, bar, result);
        }
    }
    return HintOverrideProxyStyle::subControlRect(control, option, subControl, widget);
}

int BlocksStyle::pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const
{
    switch (metric) {
        case PM_DefaultFrameWidth:
            return 2;
        case PM_ScrollBarExtent:
            return 14;
        case PM_ScrollBarSliderMin:
            return 24;
        case PM_IndicatorWidth:
        case PM_IndicatorHeight:
        case PM_ExclusiveIndicatorWidth:
        case PM_ExclusiveIndicatorHeight:
            return 18;
        case PM_MenuPanelWidth:
            return 2;
        case PM_ToolTipLabelFrameWidth:
            return 5;
        case PM_ToolBarHandleExtent:
            return 10;
        case PM_ToolBarItemSpacing:
            return 4;
        case PM_ToolBarFrameWidth:
            return 4;
        case PM_ToolBarItemMargin:
            return 2;
        case PM_SplitterWidth:
            return 6;
        case PM_TabBarBaseOverlap:
            return 1;
        case PM_SliderLength:
            return 10;
        case PM_SliderThickness:
            return 22;
        case PM_ButtonShiftHorizontal:
        case PM_ButtonShiftVertical:
            return 0;
        default:
            break;
    }
    return HintOverrideProxyStyle::pixelMetric(metric, option, widget);
}

int BlocksStyle::styleHint(StyleHint hint, const QStyleOption* option, const QWidget* widget, QStyleHintReturn* returnData) const
{
    if (hint == SH_ScrollBar_Transient) {
        return 0;
    }
    return HintOverrideProxyStyle::styleHint(hint, option, widget, returnData);
}

QSize BlocksStyle::sizeFromContents(ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget) const
{
    QSize result = HintOverrideProxyStyle::sizeFromContents(type, option, size, widget);
    switch (type) {
        case CT_PushButton:
            result.rwidth() += 8;
            result.setHeight(qMax(result.height(), 30));
            break;
        case CT_ToolButton:
            if (const auto* toolButton = qstyleoption_cast<const QStyleOptionToolButton*>(option);
                toolButton && toolButton->toolButtonStyle != Qt::ToolButtonIconOnly && !toolButton->text.isEmpty()) {
                result += QSize(10, 6);
            }
            if (widget && widget->property("_prism_primary").toBool()) {
                result.setHeight(qMax(result.height(), 46));
            }
            break;
        case CT_ComboBox:
            result.setHeight(qMax(result.height(), 30));
            break;
        default:
            break;
    }
    return result;
}
