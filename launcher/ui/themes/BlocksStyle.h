// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QColor>

#include "HintOverrideProxyStyle.h"

// Paints widgets in the bevelled, pixel-edged look of Minecraft's own interface:
// stone buttons, inventory slots, black text fields, an XP bar and purple-framed tooltips.
// Anything not handled here falls through to the wrapped style.
class BlocksStyle : public HintOverrideProxyStyle {
    Q_OBJECT

   public:
    struct Colors {
        QColor outline;
        // Raised stone buttons
        QColor button;
        QColor buttonLight;
        QColor buttonDark;
        QColor buttonDisabled;
        QColor buttonOutlineActive;
        // The one primary action on screen, such as Launch: a grass-green button
        QColor primary;
        QColor primaryLight;
        QColor primaryDark;
        // Panels: tab pages, menus, scroll bar handles
        QColor panel;
        QColor panelLight;
        QColor panelDark;
        // Text fields and item views
        QColor field;
        QColor fieldBorder;
        QColor fieldBorderFocus;
        // Sunken inventory slots: check boxes, frames around views
        QColor slot;
        QColor slotLight;
        QColor slotDark;
        // Progress bars
        QColor xp;
        QColor xpDark;
        QColor xpTrack;
        // Tooltips
        QColor tooltip;
        QColor tooltipBorderTop;
        QColor tooltipBorderBottom;
        // Drop shadow under light text on buttons
        QColor textShadow;
    };

    BlocksStyle(QStyle* base, const Colors& colors);

    void polish(QWidget* widget) override;
    using HintOverrideProxyStyle::polish;

    void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const override;
    void drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const override;
    void drawComplexControl(ComplexControl control,
                            const QStyleOptionComplex* option,
                            QPainter* painter,
                            const QWidget* widget) const override;
    void drawItemText(QPainter* painter,
                      const QRect& rect,
                      int flags,
                      const QPalette& palette,
                      bool enabled,
                      const QString& text,
                      QPalette::ColorRole textRole) const override;

    QRect subControlRect(ComplexControl control,
                         const QStyleOptionComplex* option,
                         SubControl subControl,
                         const QWidget* widget) const override;
    int pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const override;
    int styleHint(StyleHint hint, const QStyleOption* option, const QWidget* widget, QStyleHintReturn* returnData) const override;
    QSize sizeFromContents(ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget) const override;

   private:
    // Black edge, light top-left bevel, dark bottom-right bevel
    void drawRaised(QPainter* painter, QRect rect, const QColor& face, const QColor& light, const QColor& dark, const QColor& outline)
        const;
    // Dark top-left bevel, light bottom-right bevel
    void drawSunken(QPainter* painter, QRect rect, const QColor& face, const QColor& dark, const QColor& light) const;
    void drawButtonBevel(QPainter* painter, const QRect& rect, QStyle::State state, bool primary = false) const;
    void drawField(QPainter* painter, const QRect& rect, QStyle::State state, const QColor& fill) const;
    // Draws a small bitmap made of '#' characters, scaled to whole logical pixels and centred in rect
    void drawGlyph(QPainter* painter, const QRect& rect, const char* const* rows, int columns, int lines, const QColor& color) const;
    void drawArrow(QPainter* painter, const QRect& rect, Qt::ArrowType direction, const QColor& color) const;

    Colors m_colors;
    // Width of one pixel of bevel art, in logical pixels
    int m_unit = 2;
};
