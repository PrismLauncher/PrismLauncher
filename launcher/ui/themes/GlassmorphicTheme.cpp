// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2025 Prism Launcher Contributors
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
#include "GlassmorphicTheme.h"

#include <QApplication>
#include <QObject>
#include <QWidget>
#include "Application.h"
#include "ThemeManager.h"

QString GlassmorphicTheme::id()
{
    return "glassmorphic";
}

QString GlassmorphicTheme::name()
{
    return QObject::tr("Glassmorphic (macOS)");
}

QString GlassmorphicTheme::tooltip()
{
    return QObject::tr("Premium translucent theme with native macOS vibrancy and blur effects");
}

QPalette GlassmorphicTheme::colorScheme()
{
    QPalette palette;

    // Deep translucent dark tones — designed to let vibrancy shine through
    palette.setColor(QPalette::Window, QColor(30, 30, 30, 180));
    palette.setColor(QPalette::WindowText, QColor(245, 245, 247));
    palette.setColor(QPalette::Base, QColor(22, 22, 24, 160));
    palette.setColor(QPalette::AlternateBase, QColor(38, 38, 40, 160));
    palette.setColor(QPalette::ToolTipBase, QColor(40, 40, 42, 230));
    palette.setColor(QPalette::ToolTipText, QColor(245, 245, 247));
    palette.setColor(QPalette::Text, QColor(245, 245, 247));
    palette.setColor(QPalette::Button, QColor(55, 55, 58, 180));
    palette.setColor(QPalette::ButtonText, QColor(245, 245, 247));
    palette.setColor(QPalette::BrightText, QColor(255, 69, 58));  // Apple system red
    palette.setColor(QPalette::Link, QColor(100, 210, 255));       // Apple system cyan
    palette.setColor(QPalette::Highlight, QColor(50, 140, 255, 200));  // Apple system blue
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::PlaceholderText, QColor(152, 152, 157));  // Apple secondary label
    palette.setColor(QPalette::Mid, QColor(72, 72, 74));
    palette.setColor(QPalette::Dark, QColor(44, 44, 46));
    palette.setColor(QPalette::Light, QColor(99, 99, 102));

    return fadeInactive(palette, fadeAmount(), fadeColor());
}

double GlassmorphicTheme::fadeAmount()
{
    return 0.5;
}

QColor GlassmorphicTheme::fadeColor()
{
    return {30, 30, 30};
}

bool GlassmorphicTheme::hasStyleSheet()
{
    return true;
}

void GlassmorphicTheme::apply(bool initial)
{
    ITheme::apply(initial);

    // Request macOS vibrancy from ThemeManager.
    // We do NOT set WA_TranslucentBackground — that breaks Qt widget painting.
    // Instead, the NSVisualEffectView provides the blur backdrop, and Qt renders
    // its semi-transparent palette colors on top, letting vibrancy peek through
    // in the gaps between widgets.
    APPLICATION->themeManager()->enableVibrancyOnAllWindows(true);
}

QString GlassmorphicTheme::appStyleSheet()
{
    // ==========================================================================
    // Premium Glassmorphic Stylesheet v2 — Apple HIG + Modern UX
    //
    // Design system:
    //   8px grid spacing, consistent border-radius tokens,
    //   proper focus indicators, accessible contrast ratios,
    //   smooth micro-interactions, SF-style visual hierarchy
    // ==========================================================================
    return QStringLiteral(

        // -- Global foundation: system font + smooth rendering --
        "* {"
        "  font-family: -apple-system, 'SF Pro Text', 'Helvetica Neue', system-ui;"
        "}"

        // -- Main Window: fully transparent to let vibrancy show --
        "QMainWindow {"
        "  background: transparent;"
        "}"

        "QMainWindow::separator {"
        "  background: rgba(255, 255, 255, 0.06);"
        "  width: 1px;"
        "  height: 1px;"
        "}"

        "QMainWindow > QWidget#centralWidget {"
        "  background: transparent;"
        "}"

        // =====================================================================
        // TOOLBARS — frosted glass panels with 8px-grid padding
        // =====================================================================
        "QToolBar {"
        "  background: rgba(40, 40, 42, 160);"
        "  border: none;"
        "  border-bottom: 1px solid rgba(255, 255, 255, 0.08);"
        "  padding: 4px 8px;"
        "  spacing: 4px;"
        "}"

        "WideBar {"
        "  background: rgba(32, 32, 34, 170);"
        "  border: none;"
        "  border-left: 1px solid rgba(255, 255, 255, 0.06);"
        "  padding: 8px 4px;"
        "}"

        "QToolBar::handle {"
        "  background: transparent;"
        "}"

        "QToolBar::separator {"
        "  background: rgba(255, 255, 255, 0.08);"
        "  margin: 8px;"
        "  height: 1px;"
        "  width: 1px;"
        "}"

        // =====================================================================
        // TOOL BUTTONS — pill-shaped with proper focus ring
        // =====================================================================
        "QToolButton {"
        "  background: transparent;"
        "  border: 1px solid transparent;"
        "  border-radius: 8px;"
        "  padding: 6px 12px;"
        "  color: rgba(245, 245, 247, 0.82);"
        "  font-weight: 500;"
        "}"

        "QToolButton:hover {"
        "  background: rgba(255, 255, 255, 0.10);"
        "  color: rgba(245, 245, 247, 1.0);"
        "}"

        "QToolButton:pressed {"
        "  background: rgba(255, 255, 255, 0.05);"
        "  color: rgba(245, 245, 247, 0.75);"
        "}"

        "QToolButton:focus {"
        "  border: 1px solid rgba(100, 170, 255, 0.6);"
        "}"

        "QToolButton:checked {"
        "  background: rgba(50, 140, 255, 0.22);"
        "  color: rgba(100, 210, 255, 1.0);"
        "  border: 1px solid rgba(50, 140, 255, 0.15);"
        "}"

        "QToolButton:disabled {"
        "  color: rgba(245, 245, 247, 0.25);"
        "}"

        "QToolButton[popupMode=\"1\"] {"
        "  padding-right: 20px;"
        "}"

        "QToolButton::menu-indicator {"
        "  image: none;"
        "  subcontrol-position: right center;"
        "  subcontrol-origin: padding;"
        "  width: 0px;"
        "}"

        // =====================================================================
        // MENU BAR — transparent, clean spacing
        // =====================================================================
        "QMenuBar {"
        "  background: transparent;"
        "  border: none;"
        "  color: rgba(245, 245, 247, 0.85);"
        "  padding: 2px 0px;"
        "}"

        "QMenuBar::item {"
        "  background: transparent;"
        "  padding: 4px 12px;"
        "  border-radius: 6px;"
        "}"

        "QMenuBar::item:selected {"
        "  background: rgba(255, 255, 255, 0.10);"
        "}"

        "QMenuBar::item:pressed {"
        "  background: rgba(255, 255, 255, 0.15);"
        "}"

        // =====================================================================
        // MENUS — frosted glass dropdown, generous padding
        // =====================================================================
        "QMenu {"
        "  background: rgba(42, 42, 44, 235);"
        "  border: 1px solid rgba(255, 255, 255, 0.12);"
        "  border-radius: 12px;"
        "  padding: 8px;"
        "  color: rgba(245, 245, 247, 0.9);"
        "}"

        "QMenu::item {"
        "  padding: 8px 32px 8px 24px;"
        "  border-radius: 8px;"
        "  margin: 1px 4px;"
        "}"

        "QMenu::item:selected {"
        "  background: rgba(50, 140, 255, 0.35);"
        "  color: white;"
        "}"

        "QMenu::item:disabled {"
        "  color: rgba(152, 152, 157, 0.5);"
        "}"

        "QMenu::separator {"
        "  height: 1px;"
        "  background: rgba(255, 255, 255, 0.08);"
        "  margin: 6px 16px;"
        "}"

        "QMenu::icon {"
        "  padding-left: 8px;"
        "}"

        // =====================================================================
        // TOOLTIPS — compact frosted glass
        // =====================================================================
        "QToolTip {"
        "  background: rgba(50, 50, 52, 240);"
        "  color: rgba(245, 245, 247, 0.95);"
        "  border: 1px solid rgba(255, 255, 255, 0.15);"
        "  border-radius: 8px;"
        "  padding: 6px 12px;"
        "  font-size: 12px;"
        "}"

        // =====================================================================
        // SCROLLBARS — macOS Sonoma-style (8px track)
        // =====================================================================
        "QScrollBar:vertical {"
        "  background: transparent;"
        "  width: 8px;"
        "  margin: 4px 2px;"
        "  border: none;"
        "}"

        "QScrollBar::handle:vertical {"
        "  background: rgba(255, 255, 255, 0.15);"
        "  min-height: 40px;"
        "  border-radius: 3px;"
        "}"

        "QScrollBar::handle:vertical:hover {"
        "  background: rgba(255, 255, 255, 0.28);"
        "}"

        "QScrollBar::handle:vertical:pressed {"
        "  background: rgba(255, 255, 255, 0.40);"
        "}"

        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0px;"
        "}"

        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
        "  background: transparent;"
        "}"

        "QScrollBar:horizontal {"
        "  background: transparent;"
        "  height: 8px;"
        "  margin: 2px 4px;"
        "  border: none;"
        "}"

        "QScrollBar::handle:horizontal {"
        "  background: rgba(255, 255, 255, 0.15);"
        "  min-width: 40px;"
        "  border-radius: 3px;"
        "}"

        "QScrollBar::handle:horizontal:hover {"
        "  background: rgba(255, 255, 255, 0.28);"
        "}"

        "QScrollBar::handle:horizontal:pressed {"
        "  background: rgba(255, 255, 255, 0.40);"
        "}"

        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
        "  width: 0px;"
        "}"

        "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {"
        "  background: transparent;"
        "}"

        // =====================================================================
        // STATUS BAR — subtle bottom bar with proper spacing
        // =====================================================================
        "QStatusBar {"
        "  background: rgba(28, 28, 30, 180);"
        "  border-top: 1px solid rgba(255, 255, 255, 0.06);"
        "  color: rgba(152, 152, 157, 0.9);"
        "  padding: 4px 12px;"
        "  font-size: 11px;"
        "}"

        "QStatusBar::item {"
        "  border: none;"
        "}"

        "QStatusBar QLabel {"
        "  color: rgba(200, 200, 205, 0.8);"
        "  padding: 0px 4px;"
        "}"

        // =====================================================================
        // PUSH BUTTONS — glass buttons with proper min-height for touch
        // =====================================================================
        "QPushButton {"
        "  background: rgba(60, 60, 64, 180);"
        "  border: 1px solid rgba(255, 255, 255, 0.10);"
        "  border-radius: 8px;"
        "  padding: 6px 20px;"
        "  color: rgba(245, 245, 247, 0.9);"
        "  font-weight: 500;"
        "  min-height: 24px;"
        "}"

        "QPushButton:hover {"
        "  background: rgba(70, 70, 74, 200);"
        "  border: 1px solid rgba(255, 255, 255, 0.16);"
        "}"

        "QPushButton:pressed {"
        "  background: rgba(48, 48, 52, 200);"
        "}"

        "QPushButton:focus {"
        "  border: 1px solid rgba(100, 170, 255, 0.6);"
        "}"

        "QPushButton:disabled {"
        "  background: rgba(45, 45, 48, 100);"
        "  color: rgba(152, 152, 157, 0.4);"
        "  border: 1px solid rgba(255, 255, 255, 0.04);"
        "}"

        // Default / primary button — Apple blue accent with gradient
        "QPushButton:default, QPushButton[default=\"true\"] {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 rgba(55, 145, 255, 220),"
        "    stop:1 rgba(40, 120, 230, 220));"
        "  border: 1px solid rgba(80, 160, 255, 0.35);"
        "  color: white;"
        "  font-weight: 600;"
        "}"

        "QPushButton:default:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 rgba(65, 155, 255, 240),"
        "    stop:1 rgba(50, 130, 240, 240));"
        "  border: 1px solid rgba(100, 175, 255, 0.4);"
        "}"

        "QPushButton:default:pressed {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 rgba(38, 115, 225, 240),"
        "    stop:1 rgba(28, 95, 205, 240));"
        "}"

        "QPushButton:default:focus {"
        "  border: 1px solid rgba(130, 190, 255, 0.7);"
        "}"

        // Flat/tertiary buttons
        "QPushButton:flat {"
        "  background: transparent;"
        "  border: 1px solid transparent;"
        "  color: rgba(100, 210, 255, 0.9);"
        "  min-height: 24px;"
        "}"

        "QPushButton:flat:hover {"
        "  background: rgba(100, 210, 255, 0.10);"
        "  border-radius: 8px;"
        "}"

        "QPushButton:flat:focus {"
        "  border: 1px solid rgba(100, 210, 255, 0.5);"
        "}"

        // =====================================================================
        // LINE EDIT — recessed glass field with focus ring
        // =====================================================================
        "QLineEdit {"
        "  background: rgba(0, 0, 0, 0.25);"
        "  border: 1px solid rgba(255, 255, 255, 0.10);"
        "  border-radius: 8px;"
        "  padding: 6px 12px;"
        "  color: rgba(245, 245, 247, 0.95);"
        "  selection-background-color: rgba(50, 140, 255, 0.5);"
        "  selection-color: white;"
        "  min-height: 22px;"
        "}"

        "QLineEdit:hover {"
        "  border: 1px solid rgba(255, 255, 255, 0.16);"
        "}"

        "QLineEdit:focus {"
        "  border: 1px solid rgba(50, 140, 255, 0.65);"
        "  background: rgba(0, 0, 0, 0.30);"
        "}"

        "QLineEdit:disabled {"
        "  background: rgba(0, 0, 0, 0.10);"
        "  color: rgba(152, 152, 157, 0.4);"
        "  border: 1px solid rgba(255, 255, 255, 0.04);"
        "}"

        // =====================================================================
        // TEXT EDIT / PLAIN TEXT — code/log areas
        // =====================================================================
        "QTextEdit, QPlainTextEdit {"
        "  background: rgba(16, 16, 18, 210);"
        "  border: 1px solid rgba(255, 255, 255, 0.08);"
        "  border-radius: 10px;"
        "  padding: 8px;"
        "  color: rgba(245, 245, 247, 0.92);"
        "  selection-background-color: rgba(50, 140, 255, 0.45);"
        "}"

        "QTextEdit:focus, QPlainTextEdit:focus {"
        "  border: 1px solid rgba(50, 140, 255, 0.45);"
        "}"

        // =====================================================================
        // COMBO BOX — sleek dropdown with proper sizing
        // =====================================================================
        "QComboBox {"
        "  background: rgba(55, 55, 58, 180);"
        "  border: 1px solid rgba(255, 255, 255, 0.10);"
        "  border-radius: 8px;"
        "  padding: 6px 12px;"
        "  color: rgba(245, 245, 247, 0.9);"
        "  min-height: 22px;"
        "}"

        "QComboBox:hover {"
        "  border: 1px solid rgba(255, 255, 255, 0.18);"
        "}"

        "QComboBox:focus {"
        "  border: 1px solid rgba(50, 140, 255, 0.55);"
        "}"

        "QComboBox:on {"
        "  border: 1px solid rgba(50, 140, 255, 0.55);"
        "}"

        "QComboBox::drop-down {"
        "  border: none;"
        "  width: 24px;"
        "  padding-right: 4px;"
        "}"

        "QComboBox QAbstractItemView {"
        "  background: rgba(42, 42, 44, 240);"
        "  border: 1px solid rgba(255, 255, 255, 0.12);"
        "  border-radius: 10px;"
        "  padding: 6px;"
        "  selection-background-color: rgba(50, 140, 255, 0.35);"
        "  selection-color: white;"
        "  outline: none;"
        "}"

        // =====================================================================
        // SPIN BOX — consistent with line edit
        // =====================================================================
        "QSpinBox, QDoubleSpinBox {"
        "  background: rgba(0, 0, 0, 0.25);"
        "  border: 1px solid rgba(255, 255, 255, 0.10);"
        "  border-radius: 8px;"
        "  padding: 6px 10px;"
        "  color: rgba(245, 245, 247, 0.95);"
        "  min-height: 22px;"
        "}"

        "QSpinBox:hover, QDoubleSpinBox:hover {"
        "  border: 1px solid rgba(255, 255, 255, 0.16);"
        "}"

        "QSpinBox:focus, QDoubleSpinBox:focus {"
        "  border: 1px solid rgba(50, 140, 255, 0.55);"
        "}"

        "QSpinBox::up-button, QSpinBox::down-button,"
        "QDoubleSpinBox::up-button, QDoubleSpinBox::down-button {"
        "  background: rgba(255, 255, 255, 0.06);"
        "  border: none;"
        "  width: 20px;"
        "}"

        "QSpinBox::up-button:hover, QSpinBox::down-button:hover,"
        "QDoubleSpinBox::up-button:hover, QDoubleSpinBox::down-button:hover {"
        "  background: rgba(255, 255, 255, 0.12);"
        "}"

        // =====================================================================
        // CHECK BOX & RADIO BUTTON — larger targets, smooth states
        // =====================================================================
        "QCheckBox {"
        "  spacing: 8px;"
        "  color: rgba(245, 245, 247, 0.9);"
        "  padding: 2px 0px;"
        "}"

        "QCheckBox::indicator {"
        "  width: 18px;"
        "  height: 18px;"
        "  border-radius: 5px;"
        "  border: 1.5px solid rgba(255, 255, 255, 0.22);"
        "  background: rgba(0, 0, 0, 0.22);"
        "}"

        "QCheckBox::indicator:hover {"
        "  border: 1.5px solid rgba(255, 255, 255, 0.35);"
        "  background: rgba(255, 255, 255, 0.06);"
        "}"

        "QCheckBox::indicator:checked {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 rgba(55, 145, 255, 230),"
        "    stop:1 rgba(40, 120, 230, 230));"
        "  border: 1.5px solid rgba(80, 160, 255, 0.5);"
        "}"

        "QCheckBox::indicator:checked:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 rgba(65, 155, 255, 240),"
        "    stop:1 rgba(50, 130, 240, 240));"
        "}"

        "QCheckBox:disabled {"
        "  color: rgba(152, 152, 157, 0.4);"
        "}"

        "QRadioButton {"
        "  spacing: 8px;"
        "  color: rgba(245, 245, 247, 0.9);"
        "  padding: 2px 0px;"
        "}"

        "QRadioButton::indicator {"
        "  width: 18px;"
        "  height: 18px;"
        "  border-radius: 9px;"
        "  border: 1.5px solid rgba(255, 255, 255, 0.22);"
        "  background: rgba(0, 0, 0, 0.22);"
        "}"

        "QRadioButton::indicator:hover {"
        "  border: 1.5px solid rgba(255, 255, 255, 0.35);"
        "}"

        "QRadioButton::indicator:checked {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 rgba(55, 145, 255, 230),"
        "    stop:1 rgba(40, 120, 230, 230));"
        "  border: 1.5px solid rgba(80, 160, 255, 0.5);"
        "}"

        "QRadioButton:disabled {"
        "  color: rgba(152, 152, 157, 0.4);"
        "}"

        // =====================================================================
        // SLIDER — Apple-style knob with subtle shadow
        // =====================================================================
        "QSlider::groove:horizontal {"
        "  height: 4px;"
        "  background: rgba(255, 255, 255, 0.10);"
        "  border-radius: 2px;"
        "}"

        "QSlider::handle:horizontal {"
        "  background: white;"
        "  width: 18px;"
        "  height: 18px;"
        "  margin: -7px 0;"
        "  border-radius: 9px;"
        "  border: 0.5px solid rgba(0, 0, 0, 0.15);"
        "}"

        "QSlider::handle:horizontal:hover {"
        "  background: rgba(240, 240, 245, 1.0);"
        "}"

        "QSlider::handle:horizontal:pressed {"
        "  background: rgba(220, 220, 225, 1.0);"
        "}"

        "QSlider::sub-page:horizontal {"
        "  background: rgba(50, 140, 255, 0.75);"
        "  border-radius: 2px;"
        "}"

        "QSlider::groove:vertical {"
        "  width: 4px;"
        "  background: rgba(255, 255, 255, 0.10);"
        "  border-radius: 2px;"
        "}"

        "QSlider::handle:vertical {"
        "  background: white;"
        "  width: 18px;"
        "  height: 18px;"
        "  margin: 0 -7px;"
        "  border-radius: 9px;"
        "  border: 0.5px solid rgba(0, 0, 0, 0.15);"
        "}"

        // =====================================================================
        // PROGRESS BAR — luminous bar with better height
        // =====================================================================
        "QProgressBar {"
        "  background: rgba(255, 255, 255, 0.06);"
        "  border: none;"
        "  border-radius: 3px;"
        "  height: 6px;"
        "  text-align: center;"
        "  color: transparent;"
        "}"

        "QProgressBar::chunk {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 rgba(50, 140, 255, 0.9),"
        "    stop:1 rgba(100, 210, 255, 0.9));"
        "  border-radius: 3px;"
        "}"

        // =====================================================================
        // TAB WIDGET — macOS segmented control style
        // =====================================================================
        "QTabWidget::pane {"
        "  background: rgba(28, 28, 30, 150);"
        "  border: 1px solid rgba(255, 255, 255, 0.07);"
        "  border-radius: 12px;"
        "  padding: 8px;"
        "  margin-top: -1px;"
        "}"

        "QTabBar {"
        "  background: transparent;"
        "}"

        "QTabBar::tab {"
        "  background: transparent;"
        "  border: 1px solid transparent;"
        "  border-bottom: none;"
        "  border-top-left-radius: 8px;"
        "  border-top-right-radius: 8px;"
        "  padding: 8px 20px;"
        "  margin-right: 1px;"
        "  color: rgba(200, 200, 205, 0.6);"
        "  font-weight: 500;"
        "}"

        "QTabBar::tab:selected {"
        "  background: rgba(50, 140, 255, 0.18);"
        "  border: 1px solid rgba(50, 140, 255, 0.20);"
        "  border-bottom: none;"
        "  color: rgba(245, 245, 247, 1.0);"
        "  font-weight: 600;"
        "}"

        "QTabBar::tab:hover:!selected {"
        "  background: rgba(255, 255, 255, 0.06);"
        "  color: rgba(245, 245, 247, 0.8);"
        "}"

        "QTabBar::tab:focus {"
        "  border: 1px solid rgba(100, 170, 255, 0.5);"
        "  border-bottom: none;"
        "}"

        // =====================================================================
        // GROUP BOX — glass section with better title spacing
        // =====================================================================
        "QGroupBox {"
        "  background: rgba(255, 255, 255, 0.03);"
        "  border: 1px solid rgba(255, 255, 255, 0.07);"
        "  border-radius: 12px;"
        "  margin-top: 16px;"
        "  padding: 20px 16px 12px 16px;"
        "  font-weight: 600;"
        "  color: rgba(245, 245, 247, 0.9);"
        "}"

        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  subcontrol-position: top left;"
        "  padding: 2px 16px;"
        "  color: rgba(152, 152, 157, 0.9);"
        "  font-weight: 600;"
        "  font-size: 12px;"
        "  letter-spacing: 0.5px;"
        "}"

        // =====================================================================
        // LIST / TREE / TABLE — improved spacing and hover/selection
        // =====================================================================
        "QListView, QListWidget {"
        "  background: rgba(22, 22, 24, 140);"
        "  border: 1px solid rgba(255, 255, 255, 0.07);"
        "  border-radius: 10px;"
        "  padding: 4px;"
        "  outline: none;"
        "}"

        "QListView::item, QListWidget::item {"
        "  padding: 6px 12px;"
        "  border-radius: 8px;"
        "  margin: 1px 2px;"
        "  color: rgba(245, 245, 247, 0.85);"
        "}"

        "QListView::item:hover, QListWidget::item:hover {"
        "  background: rgba(255, 255, 255, 0.06);"
        "}"

        "QListView::item:selected, QListWidget::item:selected {"
        "  background: rgba(50, 140, 255, 0.28);"
        "  color: white;"
        "}"

        "QListView::item:selected:!active, QListWidget::item:selected:!active {"
        "  background: rgba(50, 140, 255, 0.15);"
        "  color: rgba(245, 245, 247, 0.9);"
        "}"

        "QTreeView, QTreeWidget {"
        "  background: rgba(22, 22, 24, 140);"
        "  border: 1px solid rgba(255, 255, 255, 0.07);"
        "  border-radius: 10px;"
        "  padding: 4px;"
        "  outline: none;"
        "  show-decoration-selected: 1;"
        "}"

        "QTreeView::item, QTreeWidget::item {"
        "  padding: 5px 8px;"
        "  border-radius: 6px;"
        "  color: rgba(245, 245, 247, 0.85);"
        "}"

        "QTreeView::item:hover, QTreeWidget::item:hover {"
        "  background: rgba(255, 255, 255, 0.05);"
        "}"

        "QTreeView::item:selected, QTreeWidget::item:selected {"
        "  background: rgba(50, 140, 255, 0.28);"
        "  color: white;"
        "}"

        "QTreeView::item:selected:!active, QTreeWidget::item:selected:!active {"
        "  background: rgba(50, 140, 255, 0.15);"
        "  color: rgba(245, 245, 247, 0.9);"
        "}"

        "QTreeView::branch {"
        "  background: transparent;"
        "}"

        "QTableView, QTableWidget {"
        "  background: rgba(22, 22, 24, 140);"
        "  border: 1px solid rgba(255, 255, 255, 0.07);"
        "  border-radius: 10px;"
        "  gridline-color: rgba(255, 255, 255, 0.05);"
        "  outline: none;"
        "}"

        "QTableView::item, QTableWidget::item {"
        "  padding: 6px 12px;"
        "  color: rgba(245, 245, 247, 0.85);"
        "}"

        "QTableView::item:selected, QTableWidget::item:selected {"
        "  background: rgba(50, 140, 255, 0.28);"
        "  color: white;"
        "}"

        "QHeaderView {"
        "  background: transparent;"
        "  border: none;"
        "}"

        "QHeaderView::section {"
        "  background: rgba(44, 44, 46, 180);"
        "  border: none;"
        "  border-right: 1px solid rgba(255, 255, 255, 0.05);"
        "  border-bottom: 1px solid rgba(255, 255, 255, 0.05);"
        "  padding: 8px 12px;"
        "  color: rgba(152, 152, 157, 0.9);"
        "  font-weight: 600;"
        "  font-size: 11px;"
        "}"

        "QHeaderView::section:hover {"
        "  background: rgba(55, 55, 58, 200);"
        "  color: rgba(200, 200, 205, 1.0);"
        "}"

        // =====================================================================
        // DIALOGS — frosted glass with better padding
        // =====================================================================
        "QDialog {"
        "  background: rgba(30, 30, 32, 220);"
        "  border-radius: 14px;"
        "}"

        "QDialogButtonBox {"
        "  button-layout: 3;"
        "}"

        // =====================================================================
        // FRAME & LABELS
        // =====================================================================
        "QFrame {"
        "  border: none;"
        "}"

        "QLabel {"
        "  color: rgba(245, 245, 247, 0.9);"
        "  background: transparent;"
        "}"

        // Secondary/descriptive labels
        "QLabel[objectName=\"labelDescription\"],"
        "QLabel[objectName=\"descriptionLabel\"] {"
        "  color: rgba(152, 152, 157, 0.85);"
        "  font-size: 12px;"
        "}"

        // =====================================================================
        // SPLITTER — minimal with wider hover target
        // =====================================================================
        "QSplitter::handle {"
        "  background: rgba(255, 255, 255, 0.05);"
        "  width: 1px;"
        "  height: 1px;"
        "}"

        "QSplitter::handle:hover {"
        "  background: rgba(50, 140, 255, 0.35);"
        "  width: 3px;"
        "  height: 3px;"
        "}"

        // =====================================================================
        // TRANSPARENT CONTAINERS
        // =====================================================================
        "QStackedWidget {"
        "  background: transparent;"
        "}"

        "QScrollArea {"
        "  background: transparent;"
        "  border: none;"
        "}"

        "QScrollArea > QWidget > QWidget {"
        "  background: transparent;"
        "}"

        // =====================================================================
        // DOCK WIDGET — consistent with tab style
        // =====================================================================
        "QDockWidget {"
        "  color: rgba(245, 245, 247, 0.9);"
        "  titlebar-close-icon: none;"
        "  titlebar-normal-icon: none;"
        "}"

        "QDockWidget::title {"
        "  background: rgba(40, 40, 42, 180);"
        "  border: 1px solid rgba(255, 255, 255, 0.06);"
        "  border-radius: 8px;"
        "  padding: 8px;"
        "  text-align: center;"
        "}"

        // =====================================================================
        // INSTANCE VIEW — transparent for vibrancy
        // =====================================================================
        "InstanceView {"
        "  background: transparent;"
        "  border: none;"
        "}"

        // =====================================================================
        // LINK LABELS — Apple system blue
        // =====================================================================
        "QLabel[linkColor] {"
        "  color: rgba(100, 210, 255, 0.95);"
        "}"

    );
}
