// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  UI/UX Modernization - Modern Light Theme
 *
 *  Design Tokens (Light):
 *  - Primary: #2563eb (Prism Blue)
 *  - Surface: #ffffff / #f8fafc / #f1f5f9
 *  - Text: #0f172a (primary) / #475569 (secondary) / #94a3b8 (muted)
 *  - Border: #e2e8f0
 *  - Radius: 8px (md), 12px (lg)
 *  - Shadow: subtle elevation
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */
#include "ModernLightTheme.h"

#include <QObject>

QString ModernLightTheme::id()
{
    return "modern-light";
}

QString ModernLightTheme::name()
{
    return QObject::tr("Modern (Light)");
}

QPalette ModernLightTheme::colorScheme()
{
    QPalette palette;

    // Base surfaces
    palette.setColor(QPalette::Window, QColor("#f8fafc"));      // Background
    palette.setColor(QPalette::WindowText, QColor("#0f172a"));  // Primary text
    palette.setColor(QPalette::Base, QColor("#ffffff"));        // Input/List background
    palette.setColor(QPalette::AlternateBase, QColor("#f1f5f9")); // Alternate row
    palette.setColor(QPalette::Text, QColor("#0f172a"));        // Text
    palette.setColor(QPalette::PlaceholderText, QColor("#94a3b8"));

    // Buttons
    palette.setColor(QPalette::Button, QColor("#ffffff"));
    palette.setColor(QPalette::ButtonText, QColor("#0f172a"));

    // Tooltips
    palette.setColor(QPalette::ToolTipBase, QColor("#1e293b"));
    palette.setColor(QPalette::ToolTipText, QColor("#f8fafc"));

    // Accent / Highlight
    palette.setColor(QPalette::Highlight, QColor("#2563eb"));       // Primary blue
    palette.setColor(QPalette::HighlightedText, QColor("#ffffff"));
    palette.setColor(QPalette::Link, QColor("#2563eb"));
    palette.setColor(QPalette::LinkVisited, QColor("#1d4ed8"));

    // Status colors
    palette.setColor(QPalette::BrightText, QColor("#dc2626"));  // Error red

    return fadeInactive(palette, fadeAmount(), fadeColor());
}

double ModernLightTheme::fadeAmount()
{
    return 0.6;
}

QColor ModernLightTheme::fadeColor()
{
    return QColor("#f8fafc");
}

bool ModernLightTheme::hasStyleSheet()
{
    return true;
}

QString ModernLightTheme::appStyleSheet()
{
    // Modern UI Stylesheet - Design System Implementation
    return R"(
/* ========== Global Reset & Base ========== */
QWidget {
    font-family: "Segoe UI", "Inter", "Noto Sans", sans-serif;
    font-size: 10pt;
    color: #0f172a;
}

/* ========== Buttons ========== */
QPushButton {
    background-color: #ffffff;
    color: #0f172a;
    border: 1px solid #e2e8f0;
    border-radius: 8px;
    padding: 8px 16px;
    font-weight: 500;
    min-height: 20px;
}

QPushButton:hover {
    background-color: #f1f5f9;
    border-color: #cbd5e1;
}

QPushButton:pressed {
    background-color: #e2e8f0;
    padding-top: 9px;
    padding-bottom: 7px;
}

QPushButton:disabled {
    background-color: #f8fafc;
    color: #94a3b8;
    border-color: #e2e8f0;
}

QPushButton:focus {
    border-color: #2563eb;
}

/* Primary button variant - using objectName="primary" */
QPushButton[objectName="primary"],
QDialogButtonBox QPushButton[dialogButtonRole="acceptButton"] {
    background-color: #2563eb;
    color: #ffffff;
    border: 1px solid #2563eb;
    font-weight: 600;
}

QPushButton[objectName="primary"]:hover,
QDialogButtonBox QPushButton[dialogButtonRole="acceptButton"]:hover {
    background-color: #1d4ed8;
    border-color: #1d4ed8;
}

QPushButton[objectName="primary"]:pressed,
QDialogButtonBox QPushButton[dialogButtonRole="acceptButton"]:pressed,
QDialogButtonBox QPushButton[dialogButtonCode="1"]:pressed {
    background-color: #1e40af;
    padding-top: 9px;
    padding-bottom: 7px;
}

/* ========== Input Fields ========== */
QLineEdit, QTextEdit, QPlainTextEdit, QSpinBox, QDoubleSpinBox, QComboBox {
    background-color: #ffffff;
    border: 1px solid #e2e8f0;
    border-radius: 8px;
    padding: 8px 12px;
    color: #0f172a;
    selection-background-color: #2563eb;
    selection-color: #ffffff;
    min-height: 20px;
}

QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus,
QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus {
    border-color: #2563eb;
    border-width: 2px;
    padding: 7px 11px; /* compensate for border width increase */
}

QLineEdit:disabled, QTextEdit:disabled, QPlainTextEdit:disabled,
QSpinBox:disabled, QDoubleSpinBox:disabled, QComboBox:disabled {
    background-color: #f8fafc;
    color: #94a3b8;
    border-color: #e2e8f0;
}

/* ========== ComboBox ========== */
QComboBox::drop-down {
    border: none;
    width: 24px;
}

QComboBox::down-arrow {
    image: none;
    border-left: 4px solid transparent;
    border-right: 4px solid transparent;
    border-top: 5px solid #64748b;
    width: 0;
    height: 0;
    margin-right: 8px;
}

QComboBox QAbstractItemView {
    background-color: #ffffff;
    border: 1px solid #e2e8f0;
    border-radius: 8px;
    padding: 4px;
    selection-background-color: #f1f5f9;
    selection-color: #0f172a;
    outline: none;
}

/* ========== CheckBox & RadioButton ========== */
QCheckBox, QRadioButton {
    spacing: 8px;
    padding: 4px 0;
}

QCheckBox::indicator, QRadioButton::indicator {
    width: 18px;
    height: 18px;
    border: 2px solid #cbd5e1;
    border-radius: 4px;
    background-color: #ffffff;
}

QRadioButton::indicator {
    border-radius: 9px;
}

QCheckBox::indicator:hover, QRadioButton::indicator:hover {
    border-color: #2563eb;
}

QCheckBox::indicator:checked, QRadioButton::indicator:checked {
    background-color: #2563eb;
    border-color: #2563eb;
}

QCheckBox::indicator:checked {
    image: none;
}

QRadioButton::indicator:checked::indicator {
    background-color: #ffffff;
}

QCheckBox:disabled, QRadioButton:disabled {
    color: #94a3b8;
}

QCheckBox::indicator:disabled, QRadioButton::indicator:disabled {
    border-color: #e2e8f0;
    background-color: #f8fafc;
}

/* ========== Scrollbars ========== */
QScrollBar:vertical {
    background: transparent;
    width: 10px;
    margin: 2px;
    border-radius: 5px;
}

QScrollBar::handle:vertical {
    background: #cbd5e1;
    border-radius: 5px;
    min-height: 40px;
}

QScrollBar::handle:vertical:hover {
    background: #94a3b8;
}

QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0px;
}

QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
    background: none;
}

QScrollBar:horizontal {
    background: transparent;
    height: 10px;
    margin: 2px;
    border-radius: 5px;
}

QScrollBar::handle:horizontal {
    background: #cbd5e1;
    border-radius: 5px;
    min-width: 40px;
}

QScrollBar::handle:horizontal:hover {
    background: #94a3b8;
}

QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
    width: 0px;
}

QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
    background: none;
}

/* ========== Tab Widget ========== */
QTabWidget::pane {
    border: 1px solid #e2e8f0;
    border-radius: 0 8px 8px 8px;
    background-color: #ffffff;
    top: -1px;
}

QTabBar::tab {
    background-color: transparent;
    border: none;
    padding: 10px 20px;
    margin-right: 4px;
    color: #64748b;
    font-weight: 500;
    border-bottom: 2px solid transparent;
}

QTabBar::tab:hover {
    color: #0f172a;
    background-color: #f1f5f9;
    border-top-left-radius: 8px;
    border-top-right-radius: 8px;
}

QTabBar::tab:selected {
    color: #2563eb;
    border-bottom: 2px solid #2563eb;
    font-weight: 600;
}

/* ========== List & Tree Views ========== */
QListView, QTreeView, QTableView {
    background-color: #ffffff;
    border: 1px solid #e2e8f0;
    border-radius: 8px;
    padding: 4px;
    selection-background-color: #eff6ff;
    selection-color: #1e40af;
    outline: none;
}

QListView::item, QTreeView::item, QTableView::item {
    padding: 8px 12px;
    border-radius: 6px;
    margin: 2px;
}

QListView::item:hover, QTreeView::item:hover, QTableView::item:hover {
    background-color: #f8fafc;
}

QListView::item:selected, QTreeView::item:selected, QTableView::item:selected {
    background-color: #eff6ff;
    color: #1e40af;
}

QHeaderView::section {
    background-color: #f8fafc;
    border: none;
    border-bottom: 1px solid #e2e8f0;
    padding: 10px 12px;
    font-weight: 600;
    color: #475569;
}

/* ========== GroupBox ========== */
QGroupBox {
    border: 1px solid #e2e8f0;
    border-radius: 12px;
    margin-top: 16px;
    padding: 16px;
    background-color: #ffffff;
}

QGroupBox::title {
    subcontrol-origin: margin;
    left: 16px;
    padding: 0 8px;
    font-weight: 600;
    color: #0f172a;
}

/* ========== ToolTips ========== */
QToolTip {
    background-color: #1e293b;
    color: #f8fafc;
    border: none;
    border-radius: 6px;
    padding: 8px 12px;
    font-size: 9pt;
}

/* ========== Menu & MenuBar ========== */
QMenuBar {
    background-color: #ffffff;
    border-bottom: 1px solid #e2e8f0;
    padding: 2px 8px;
}

QMenuBar::item {
    padding: 6px 12px;
    border-radius: 6px;
    background: transparent;
}

QMenuBar::item:selected {
    background-color: #f1f5f9;
}

QMenu {
    background-color: #ffffff;
    border: 1px solid #e2e8f0;
    border-radius: 8px;
    padding: 8px;
}

QMenu::item {
    padding: 8px 24px 8px 28px;
    border-radius: 6px;
    min-width: 180px;
}

QMenu::item:selected {
    background-color: #f1f5f9;
}

QMenu::separator {
    height: 1px;
    background: #e2e8f0;
    margin: 6px 8px;
}

/* ========== Progress Bar ========== */
QProgressBar {
    border: none;
    background-color: #e2e8f0;
    border-radius: 999px;
    height: 8px;
    text-align: center;
    color: transparent;
}

QProgressBar::chunk {
    background-color: #2563eb;
    border-radius: 999px;
}

/* ========== Slider ========== */
QSlider::groove:horizontal {
    height: 4px;
    background: #e2e8f0;
    border-radius: 2px;
}

QSlider::handle:horizontal {
    background: #2563eb;
    border: 2px solid #ffffff;
    width: 16px;
    height: 16px;
    margin: -6px 0;
    border-radius: 9px;
    box-shadow: 0 1px 3px rgba(0,0,0,0.1);
}

QSlider::handle:horizontal:hover {
    width: 20px;
    height: 20px;
    margin: -8px 0;
    border-radius: 10px;
}

QSlider::handle:horizontal:pressed {
    background: #1d4ed8;
    width: 18px;
    height: 18px;
    margin: -7px 0;
    border-radius: 9px;
}

/* ========== Status Bar ========== */
QStatusBar {
    background-color: #ffffff;
    border-top: 1px solid #e2e8f0;
    color: #64748b;
    padding: 4px 8px;
}

/* ========== ToolBar ========== */
QToolBar {
    background-color: #ffffff;
    border: none;
    border-bottom: 1px solid #e2e8f0;
    padding: 4px 8px;
    spacing: 4px;
}

QToolBar::separator {
    background-color: #e2e8f0;
    width: 1px;
    margin: 8px 8px;
}

QToolButton {
    background-color: transparent;
    border: 1px solid transparent;
    border-radius: 8px;
    padding: 6px 10px;
    color: #475569;
}

QToolButton:hover {
    background-color: #f1f5f9;
    border-color: #e2e8f0;
    color: #0f172a;
}

QToolButton:pressed {
    background-color: #e2e8f0;
}

QToolButton:checked {
    background-color: #eff6ff;
    border-color: #bfdbfe;
    color: #1d4ed8;
}

QToolButton:disabled {
    color: #94a3b8;
}

QToolButton:focus {
    border-color: #2563eb;
}

/* Primary CTA tool button */
QToolButton[objectName="primary"] {
    background-color: #2563eb;
    border: 1px solid #2563eb;
    color: #ffffff;
    font-weight: 600;
}

QToolButton[objectName="primary"]:hover {
    background-color: #1d4ed8;
    border-color: #1d4ed8;
    color: #ffffff;
}

QToolButton[objectName="primary"]:pressed {
    background-color: #1e40af;
    border-color: #1e40af;
}

QToolButton[objectName="primary"]:disabled {
    background-color: #94a3b8;
    border-color: #94a3b8;
    color: #e2e8f0;
}

/* Vertical toolbar (instance sidebar) */
QToolBar[orientation="1"] {
    border-bottom: none;
    border-right: 1px solid #e2e8f0;
    padding: 12px 8px;
}

QToolBar[orientation="1"] QToolButton {
    padding: 10px 8px;
    min-width: 80px;
}

/* ========== Splitter ========== */
QSplitter::handle {
    background-color: #e2e8f0;
    width: 1px;
}

QSplitter::handle:hover {
    background-color: #2563eb;
}

/* ========== Dock Widget ========== */
QDockWidget {
    titlebar-close-icon: none;
    titlebar-normal-icon: none;
}

QDockWidget::title {
    background-color: #f8fafc;
    border-bottom: 1px solid #e2e8f0;
    padding: 8px 12px;
    font-weight: 600;
}

/* ========== Tool Box ========== */
QToolBox::tab {
    background-color: #f8fafc;
    border: 1px solid #e2e8f0;
    border-radius: 8px;
    padding: 12px;
    font-weight: 500;
}

QToolBox::tab:selected {
    background-color: #ffffff;
    border-color: #2563eb;
    color: #2563eb;
    font-weight: 600;
}

/* ========== Settings / Page Navigation ========== */
PageView {
    background-color: #f8fafc;
    border: none;
    border-right: 1px solid #e2e8f0;
    padding: 8px 4px;
    outline: none;
}

PageView::item {
    border-radius: 8px;
    padding: 8px 12px;
    margin: 2px 4px;
    color: #475569;
}

PageView::item:hover {
    background-color: #f1f5f9;
    color: #0f172a;
}

PageView::item:selected {
    background-color: #eff6ff;
    color: #1d4ed8;
    font-weight: 500;
}

PageView::item:selected:active {
    background-color: #dbeafe;
}

/* Search field in settings */
PageContainer QLineEdit {
    margin: 8px;
    padding: 8px 12px;
    border-radius: 8px;
}

/* Settings page content area */
QScrollArea {
    border: none;
    background-color: #ffffff;
}

QScrollArea > QWidget > QWidget {
    background-color: #ffffff;
}

/* Settings group sections */
QGroupBox {
    margin-top: 16px;
    padding-top: 16px;
    padding-left: 16px;
    padding-right: 16px;
    padding-bottom: 12px;
    border-radius: 12px;
    border: 1px solid #e2e8f0;
    background-color: #fafafa;
    font-weight: 600;
    color: #0f172a;
}

QGroupBox::title {
    subcontrol-origin: margin;
    left: 16px;
    padding: 0 8px;
    color: #0f172a;
}

/* ========== Dialog Buttons ========== */
QDialogButtonBox {
    padding: 12px 16px;
    border-top: 1px solid #e2e8f0;
    background-color: #f8fafc;
}

QDialogButtonBox QPushButton {
    min-width: 80px;
    padding: 6px 16px;
}

QDialogButtonBox QPushButton[dialogButtonCode="1"] {
    background-color: #2563eb;
    color: white;
    border: 1px solid #2563eb;
}

QDialogButtonBox QPushButton[dialogButtonCode="1"]:hover {
    background-color: #1d4ed8;
    border-color: #1d4ed8;
}

/* ========== Instance View Specific ========== */
InstanceView {
    background-color: #f8fafc;
    border: none;
}

)";
}

QString ModernLightTheme::tooltip()
{
    return QObject::tr("Modern UI design system with clean aesthetics and improved spacing");
}
