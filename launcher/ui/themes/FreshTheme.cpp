// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Agustin <agustinm28@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "FreshTheme.h"

#include <QApplication>
#include <QFile>
#include <QHash>
#include <QMenu>
#include <QPainter>
#include <QRegularExpression>
#include <QStyleFactory>
#include <QToolButton>

#include "Application.h"
#include "HintOverrideProxyStyle.h"
#include "ThemeManager.h"
#include "settings/SettingsObject.h"

namespace {
// Design tokens: the single source for the palette and for every @token in fresh.qss
// Warm, earthy darks picked to sit well with the Simple (Colored) icons (green #39b54a, brown #8c6239, off-white #f2f2f2)
const QHash<QString, QString> TOKENS{
    // surfaces, darkest to lightest
    { "window", "#171513" },
    { "base", "#1c1a17" },
    { "alternate-base", "#211e1b" },
    { "card", "#221f1b" },
    { "surface", "#2b2723" },
    { "surface-hover", "#36312b" },
    { "surface-pressed", "#403a33" },
    { "border", "#3e362e" },
    { "border-subtle", "#2a251f" },
    // text
    { "text", "#efebe4" },
    { "text-muted", "#aaa194" },
    { "text-disabled", "#655d53" },
    { "bright-text", "#e0484e" },
    { "link", "#6699cc" },
    // accent: the icons' green
    { "accent", "#39b54a" },
    { "accent-hover", "#4ec65e" },
    { "accent-pressed", "#2e963c" },
    { "accent-soft", "rgba(57, 181, 74, 40)" },
    { "accent-soft-strong", "rgba(57, 181, 74, 70)" },
    { "on-accent", "#0e1a0f" },
    // Material You tonal containers, derived from the accent green over the warm darks
    { "primary-container", "#1f3a22" },
    { "on-primary-container", "#b9f0b6" },
    { "secondary-container", "#343d2f" },
    { "on-secondary-container", "#dbe7d1" },
    // secondary container with the hover/pressed state layer baked in (QSS can't stack a layer on a fill)
    { "secondary-container-hover", "#3e4838" },
    { "secondary-container-pressed", "#48533f" },
    // state layers: translucent on-surface over whatever is below (hover 8%, pressed 12%)
    { "state-hover", "rgba(239, 235, 228, 20)" },
    { "state-pressed", "rgba(239, 235, 228, 31)" },
    { "inverse-surface", "#e9e4dc" },
    { "inverse-on-surface", "#2b2723" },
    // secondary: the icons' brown
    { "earth", "#8c6239" },
    { "earth-soft", "rgba(140, 98, 57, 60)" },
    // shape
    { "radius-xl", "24px" },
    { "radius-lg", "20px" },
    { "radius", "12px" },
    { "radius-sm", "10px" },
    // full pill for controls 32px or taller (a radius above half the height renders wrong)
    { "radius-pill", "16px" },
    // spacing
    { "pad-lg", "12px" },
    { "pad-y", "7px" },
    { "pad-x", "16px" },
    { "pad-tight", "4px" },
    { "item-pad-y", "6px" },
    { "item-pad-x", "10px" },
    { "margin", "2px" },
    { "spacing", "4px" },
};

QColor token(const QString& name)
{
    return QColor(TOKENS.value(name));
}

// Styled tool buttons hardcode a 4px gap between icon and text. Their label text still goes through
// the app style's drawItemText, so push it right there.
class FreshStyle : public HintOverrideProxyStyle {
   public:
    using HintOverrideProxyStyle::HintOverrideProxyStyle;

    static constexpr int EXTRA_ICON_TEXT_GAP = 8;  // stylesheets add this to the right padding of such buttons

    void drawItemText(QPainter* painter,
                      const QRect& rect,
                      int flags,
                      const QPalette& palette,
                      bool enabled,
                      const QString& text,
                      QPalette::ColorRole textRole) const override
    {
        auto* button = dynamic_cast<QToolButton*>(painter->device());
        const bool besideIcon = button != nullptr && button->toolButtonStyle() == Qt::ToolButtonTextBesideIcon && !button->icon().isNull();
        HintOverrideProxyStyle::drawItemText(painter, besideIcon ? rect.adjusted(EXTRA_ICON_TEXT_GAP, 0, 0, 0) : rect, flags, palette,
                                             enabled, text, textRole);
    }
};
// Popup menus are rectangular native windows, so a stylesheet radius leaves square corners behind it.
// Under Fresh, make them translucent (and drop the rectangular native shadow) before they are first shown.
class RoundedPopups : public QObject {
   protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() == QEvent::Polish && FreshTheme::isActive()) {
            if (auto* menu = qobject_cast<QMenu*>(watched); menu != nullptr && !menu->testAttribute(Qt::WA_WState_Created)) {
                // on Windows the translucent corners render black unless the window is also frameless
                menu->setWindowFlag(Qt::FramelessWindowHint);
                menu->setWindowFlag(Qt::NoDropShadowWindowHint);
                menu->setAttribute(Qt::WA_TranslucentBackground);
            }
        }
        return false;
    }
};
}  // namespace

void FreshTheme::apply(bool initial)
{
    FusionTheme::apply(initial);

    static auto* roundedPopups = [] {
        auto* filter = new RoundedPopups;
        qApp->installEventFilter(filter);
        return filter;
    }();
    Q_UNUSED(roundedPopups);
}

QStyle* FreshTheme::createStyle()
{
    return new FreshStyle(QStyleFactory::create(qtTheme()));
}

QString FreshTheme::id()
{
    return "fresh";
}

QString FreshTheme::name()
{
    return "Fresh";
}

QString FreshTheme::tooltip()
{
    return QObject::tr("Prism Launcher with a fresh coat of paint");
}

bool FreshTheme::hasStyleSheet()
{
    return true;
}

QString FreshTheme::appStyleSheet()
{
    QFile file(":/themes/fresh/fresh.qss");
    if (!file.open(QIODevice::ReadOnly)) {
        themeWarningLog() << "Couldn't load Fresh stylesheet:" << file.errorString();
        return {};
    }

    const QString source = QString::fromUtf8(file.readAll());
    static const QRegularExpression tokenPattern("@([a-z-]+)");

    QString result;
    qsizetype last = 0;
    for (const auto& match : tokenPattern.globalMatch(source)) {
        result += source.mid(last, match.capturedStart() - last);
        result += TOKENS.value(match.captured(1), match.captured(0));
        last = match.capturedEnd();
    }
    result += source.mid(last);
    return result;
}

QPalette FreshTheme::colorScheme()
{
    QPalette palette;
    palette.setColor(QPalette::Window, token("window"));
    palette.setColor(QPalette::WindowText, token("text"));
    palette.setColor(QPalette::Base, token("base"));
    palette.setColor(QPalette::AlternateBase, token("alternate-base"));
    palette.setColor(QPalette::ToolTipBase, token("surface"));
    palette.setColor(QPalette::ToolTipText, token("text"));
    palette.setColor(QPalette::Text, token("text"));
    palette.setColor(QPalette::Button, token("surface"));
    palette.setColor(QPalette::ButtonText, token("text"));
    palette.setColor(QPalette::BrightText, token("bright-text"));
    palette.setColor(QPalette::Link, token("link"));
    palette.setColor(QPalette::Highlight, token("accent"));
    palette.setColor(QPalette::HighlightedText, token("on-accent"));
    palette.setColor(QPalette::PlaceholderText, token("text-muted"));
    return fadeInactive(palette, fadeAmount(), fadeColor());
}

double FreshTheme::fadeAmount()
{
    return 0.5;
}

QColor FreshTheme::fadeColor()
{
    return token("window");
}

bool FreshTheme::isActive()
{
    return APPLICATION->settings()->get("ApplicationTheme").toString() == "fresh";
}

QColor FreshTheme::color(const QString& name)
{
    return token(name);
}
