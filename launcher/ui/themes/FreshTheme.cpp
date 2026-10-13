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
#include <QGraphicsOpacityEffect>
#include <QHash>
#include <QMenu>
#include <QPainter>
#include <QPropertyAnimation>
#include <QRegularExpression>
#include <QStyleFactory>
#include <QToolButton>

#include "Application.h"
#include "HintOverrideProxyStyle.h"
#include "ThemeManager.h"
#include "settings/SettingsObject.h"

namespace {
// Design tokens: the single source for the palette and for every @token in fresh.qss.
// shape and spacing, shared by every palette
const QHash<QString, QString> SHARED_TOKENS{
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

// Terra: warm, earthy darks picked to sit well with the Simple (Colored) icons (green #39b54a, brown #8c6239)
const QHash<QString, QString> TERRA_TOKENS{
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
    // Material You tonal containers, derived from the accent over the darks
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
    // tint for highlights like the hero card: the icons' brown
    { "tint", "#8c6239" },
    { "tint-soft", "rgba(140, 98, 57, 60)" },
    // suffix of the palette's SVG variants (url(...-@icons.svg))
    { "icons", "terra" },
};

// Stone: neutral darks with a slight blue cast, as in most modern apps, with a bright green accent
const QHash<QString, QString> STONE_TOKENS{
    // surfaces, darkest to lightest
    { "window", "#16181c" },
    { "base", "#1a1c20" },
    { "alternate-base", "#1e2025" },
    { "card", "#1f2126" },
    { "surface", "#27292e" },
    { "surface-hover", "#303338" },
    { "surface-pressed", "#393c42" },
    { "border", "#33363d" },
    { "border-subtle", "#25282d" },
    // text
    { "text", "#e2e4e9" },
    { "text-muted", "#9aa0aa" },
    { "text-disabled", "#5c616b" },
    { "bright-text", "#ff6b6b" },
    { "link", "#4f9cff" },
    // accent: bright green
    { "accent", "#1bd96a" },
    { "accent-hover", "#3ee584" },
    { "accent-pressed", "#14b356" },
    { "accent-soft", "rgba(27, 217, 106, 38)" },
    { "accent-soft-strong", "rgba(27, 217, 106, 70)" },
    { "on-accent", "#0b1f12" },
    // Material You tonal containers: the accent for primary, cool neutral gray for secondary
    { "primary-container", "#163a24" },
    { "on-primary-container", "#b2f5c9" },
    { "secondary-container", "#2e333b" },
    { "on-secondary-container", "#dfe4ec" },
    // secondary container with the hover/pressed state layer baked in (QSS can't stack a layer on a fill)
    { "secondary-container-hover", "#383e47" },
    { "secondary-container-pressed", "#424955" },
    // state layers: translucent on-surface over whatever is below (hover 8%, pressed 12%)
    { "state-hover", "rgba(226, 228, 233, 20)" },
    { "state-pressed", "rgba(226, 228, 233, 31)" },
    { "inverse-surface", "#e2e4e9" },
    { "inverse-on-surface", "#27292e" },
    // tint for highlights like the hero card: slate blue
    { "tint", "#5b6b80" },
    { "tint-soft", "rgba(91, 107, 128, 60)" },
    // suffix of the palette's SVG variants (url(...-@icons.svg))
    { "icons", "stone" },
};

QString tokenValue(FreshTheme::Palette palette, const QString& name)
{
    const auto& colors = palette == FreshTheme::Palette::Stone ? STONE_TOKENS : TERRA_TOKENS;
    return colors.value(name, SHARED_TOKENS.value(name));
}

FreshTheme::Palette activePalette()
{
    return APPLICATION->settings()->get("ApplicationTheme").toString() == "fresh_stone" ? FreshTheme::Palette::Stone
                                                                                        : FreshTheme::Palette::Terra;
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
// Open animations for every window: windows, dialogs and tooltips fade in; popups (menus, combo box
// lists) fade in while sliding a few pixels. Short property animations, nothing runs once they end.
constexpr int WINDOW_FADE_MS = 180;
constexpr int POPUP_FADE_MS = 130;
constexpr int TOOLTIP_FADE_MS = 110;
constexpr int POPUP_SLIDE_PX = 6;
constexpr auto WINDOW_ANIMATION_NAME = "freshWindowAnimation";

void stopWindowAnimations(QWidget* window)
{
    for (auto* animation : window->findChildren<QPropertyAnimation*>(WINDOW_ANIMATION_NAME, Qt::FindDirectChildrenOnly))
        animation->stop();  // deletes itself (DeleteWhenStopped)
}

void startWindowAnimation(QWidget* window, const QByteArray& property, const QVariant& from, const QVariant& to, int duration)
{
    auto* animation = new QPropertyAnimation(window, property, window);
    animation->setObjectName(WINDOW_ANIMATION_NAME);
    animation->setDuration(duration);
    animation->setStartValue(from);
    animation->setEndValue(to);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void animateWindowIn(QWidget* window, int duration, int slide)
{
    stopWindowAnimations(window);
    // the Show event arrives before the native window is mapped, so it never flashes at full opacity
    window->setWindowOpacity(0.0);
    startWindowAnimation(window, "windowOpacity", 0.0, 1.0, duration);
    if (slide != 0)
        startWindowAnimation(window, "pos", window->pos() - QPoint(0, slide), window->pos(), duration);
}

// Window-level effects of the theme: rounded popups and open animations
class WindowEffects : public QObject {
   protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        const auto type = event->type();
        if (type != QEvent::Polish && type != QEvent::Show && type != QEvent::Hide)
            return false;
        auto* widget = qobject_cast<QWidget*>(watched);
        if (widget == nullptr || !widget->isWindow())
            return false;

        if (type == QEvent::Hide) {
            // a window closed mid-animation must not stay half transparent (e.g. after switching themes)
            stopWindowAnimations(widget);
            if (widget->windowOpacity() < 1.0)
                widget->setWindowOpacity(1.0);
            return false;
        }
        if (!FreshTheme::isActive())
            return false;

        if (type == QEvent::Polish) {
            // popup menus are rectangular native windows, so a stylesheet radius leaves square corners behind it;
            // make them translucent (and drop the rectangular native shadow) before they are first shown.
            // On Windows the translucent corners render black unless the window is also frameless
            // Same for the popup window of combo boxes, whose list view has rounded corners
            const bool roundedPopup = qobject_cast<QMenu*>(widget) != nullptr || widget->inherits("QComboBoxPrivateContainer");
            if (roundedPopup && !widget->testAttribute(Qt::WA_WState_Created)) {
                widget->setWindowFlag(Qt::FramelessWindowHint);
                widget->setWindowFlag(Qt::NoDropShadowWindowHint);
                widget->setAttribute(Qt::WA_TranslucentBackground);
            }
            return false;
        }

        // Show: skip spontaneous ones (e.g. restoring a minimized window)
        if (event->spontaneous())
            return false;
        switch (widget->windowType()) {
            case Qt::Window:
            case Qt::Dialog:
                animateWindowIn(widget, WINDOW_FADE_MS, 0);
                break;
            case Qt::Popup:
                animateWindowIn(widget, POPUP_FADE_MS, POPUP_SLIDE_PX);
                break;
            case Qt::ToolTip:
                animateWindowIn(widget, TOOLTIP_FADE_MS, 0);
                break;
            default:
                break;
        }
        return false;
    }
};
}  // namespace

void FreshTheme::apply(bool initial)
{
    FusionTheme::apply(initial);

    // Qt's own combo box roll-out (on when the system enables it) fights the popup fade and flickers;
    // it is a global switch, so other themes keep it off too until restart
    QApplication::setEffectEnabled(Qt::UI_AnimateCombo, false);

    static auto* windowEffects = [] {
        auto* filter = new WindowEffects;
        qApp->installEventFilter(filter);
        return filter;
    }();
    Q_UNUSED(windowEffects);
}

void FreshTheme::fadeIn(QWidget* widget, int duration)
{
    if (!isActive() || widget == nullptr || widget->graphicsEffect() != nullptr)
        return;
    // graphics effects don't work with OpenGL widgets (e.g. the skin preview)
    for (auto* child : widget->findChildren<QWidget*>()) {
        if (child->inherits("QOpenGLWidget"))
            return;
    }

    auto* effect = new QGraphicsOpacityEffect(widget);
    effect->setOpacity(0.0);
    widget->setGraphicsEffect(effect);

    // parented to the widget, not the effect: the effect is deleted when the animation finishes
    auto* animation = new QPropertyAnimation(effect, "opacity", widget);
    animation->setDuration(duration);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    QObject::connect(animation, &QPropertyAnimation::finished, widget, [widget, effect] {
        // the effect renders the widget through an offscreen pixmap, drop it once fully visible
        if (widget->graphicsEffect() == effect)
            widget->setGraphicsEffect(nullptr);
    });
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

QStyle* FreshTheme::createStyle()
{
    return new FreshStyle(QStyleFactory::create(qtTheme()));
}

FreshTheme::FreshTheme(Palette palette) : m_palette(palette) {}

QString FreshTheme::id()
{
    return m_palette == Palette::Stone ? "fresh_stone" : "fresh_terra";
}

QString FreshTheme::name()
{
    return m_palette == Palette::Stone ? "Fresh (Stone)" : "Fresh (Terra)";
}

QString FreshTheme::tooltip()
{
    return m_palette == Palette::Stone ? QObject::tr("Prism Launcher with a fresh coat of paint, in cool stone grays")
                                       : QObject::tr("Prism Launcher with a fresh coat of paint, in warm earth tones");
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
        const QString value = tokenValue(m_palette, match.captured(1));
        result += value.isEmpty() ? match.captured(0) : value;
        last = match.capturedEnd();
    }
    result += source.mid(last);
    return result;
}

QPalette FreshTheme::colorScheme()
{
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(tokenValue(m_palette, "window")));
    palette.setColor(QPalette::WindowText, QColor(tokenValue(m_palette, "text")));
    palette.setColor(QPalette::Base, QColor(tokenValue(m_palette, "base")));
    palette.setColor(QPalette::AlternateBase, QColor(tokenValue(m_palette, "alternate-base")));
    palette.setColor(QPalette::ToolTipBase, QColor(tokenValue(m_palette, "surface")));
    palette.setColor(QPalette::ToolTipText, QColor(tokenValue(m_palette, "text")));
    palette.setColor(QPalette::Text, QColor(tokenValue(m_palette, "text")));
    palette.setColor(QPalette::Button, QColor(tokenValue(m_palette, "surface")));
    palette.setColor(QPalette::ButtonText, QColor(tokenValue(m_palette, "text")));
    palette.setColor(QPalette::BrightText, QColor(tokenValue(m_palette, "bright-text")));
    palette.setColor(QPalette::Link, QColor(tokenValue(m_palette, "link")));
    palette.setColor(QPalette::Highlight, QColor(tokenValue(m_palette, "accent")));
    palette.setColor(QPalette::HighlightedText, QColor(tokenValue(m_palette, "on-accent")));
    palette.setColor(QPalette::PlaceholderText, QColor(tokenValue(m_palette, "text-muted")));
    return fadeInactive(palette, fadeAmount(), fadeColor());
}

double FreshTheme::fadeAmount()
{
    return 0.5;
}

QColor FreshTheme::fadeColor()
{
    return QColor(tokenValue(m_palette, "window"));
}

bool FreshTheme::isActive()
{
    return APPLICATION->settings()->get("ApplicationTheme").toString().startsWith("fresh_");
}

QColor FreshTheme::color(const QString& name)
{
    return QColor(tokenValue(activePalette(), name));
}
