// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  UI/UX Modernization - Modern Dark Theme
 *
 *  Design Tokens (Dark):
 *  - Primary: #3b82f6 (Prism Blue - lighter for dark mode)
 *  - Surface: #0f172a / #1e293b / #334155
 *  - Text: #f1f5f9 (primary) / #cbd5e1 (secondary) / #64748b (muted)
 *  - Border: #334155
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
#include "ModernDarkTheme.h"

#include <QObject>

QString ModernDarkTheme::id()
{
    return "modern-dark";
}

QString ModernDarkTheme::name()
{
    return QObject::tr("Modern (Dark)");
}

QPalette ModernDarkTheme::colorScheme()
{
    QPalette palette;

    // Base surfaces - Dark slate palette
    palette.setColor(QPalette::Window, QColor("#0f172a"));      // Deep background
    palette.setColor(QPalette::WindowText, QColor("#f1f5f9"));  // Primary text
    palette.setColor(QPalette::Base, QColor("#1e293b"));        // Card/Input background
    palette.setColor(QPalette::AlternateBase, QColor("#334155")); // Alternate row
    palette.setColor(QPalette::Text, QColor("#f1f5f9"));        // Text
    palette.setColor(QPalette::PlaceholderText, QColor("#64748b"));

    // Buttons
    palette.setColor(QPalette::Button, QColor("#334155"));
    palette.setColor(QPalette::ButtonText, QColor("#f1f5f9"));

    // Tooltips
    palette.setColor(QPalette::ToolTipBase, QColor("#334155"));
    palette.setColor(QPalette::ToolTipText, QColor("#f1f5f9"));

    // Accent / Highlight
    palette.setColor(QPalette::Highlight, QColor("#3b82f6"));       // Primary blue
    palette.setColor(QPalette::HighlightedText, QColor("#ffffff"));
    palette.setColor(QPalette::Link, QColor("#60a5fa"));
    palette.setColor(QPalette::LinkVisited, QColor("#3b82f6"));

    // Status colors
    palette.setColor(QPalette::BrightText, QColor("#ef4444"));  // Error red

    return fadeInactive(palette, fadeAmount(), fadeColor());
}

double ModernDarkTheme::fadeAmount()
{
    return 0.5;
}

QColor ModernDarkTheme::fadeColor()
{
    return QColor("#0f172a");
}

bool ModernDarkTheme::hasStyleSheet()
{
    return true;
}

QString ModernDarkTheme::appStyleSheet()
{
    // Modern Dark UI Stylesheet - Design System Implementation
    return R"(
/* ========== Global Reset & Base ========== */
QWidget {
    font-family: "Segoe UI", "Inter", "Noto Sans", sans-serif;
    font-size: 10pt;
    color: #f1f5f9;
}

/* ========== Buttons ========== */
QPushButton {
    background-color: #334155;
    color: #f1f5f9;
    border: 1px solid #475569;
    border-radius: 8px;
    padding: 8px 16px;
    font-weight: 500;
    min-height: 20px;
}

QPushButton:hover {
    background-color: #475569;
    border-color: #64748b;
}

QPushButton:pressed {
    background-color: #1e293b;
    padding-top: 9px;
    padding-bottom: 7px;
}

QPushButton:disabled {
    background-color: #1e293b;
    color: #64748b;
    border-color: #334155;
}

QPushButton:focus {
    border-color: #3b82f6;
}

/* Primary button variant */
QPushButton[objectName="primary"],
QDialogButtonBox QPushButton[dialogButtonRole="acceptButton"] {
    background-color: #3b82f6;
    color: #ffffff;
    border: 1px solid #3b82f6;
    font-weight: 600;
}

QPushButton[objectName="primary"]:hover,
QDialogButtonBox QPushButton[dialogButtonRole="acceptButton"]:hover {
    background-color: #2563eb;
    border-color: #2563eb;
}

QPushButton[objectName="primary"]:pressed,
QDialogButtonBox QPushButton[dialogButtonRole="acceptButton"]:pressed,
QDialogButtonBox QPushButton[dialogButtonCode="1"]:pressed {
    background-color: #1d4ed8;
    padding-top: 9px;
    padding-bottom: 7px;
}

/* ========== Input Fields ========== */
QLineEdit, QTextEdit, QPlainTextEdit, QSpinBox, QDoubleSpinBox, QComboBox {
    background-color: #1e293b;
    border: 1px solid #334155;
    border-radius: 8px;
    padding: 8px 12px;
    color: #f1f5f9;
    selection-background-color: #3b82f6;
    selection-color: #ffffff;
    min-height: 20px;
}

QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus,
QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus {
    border-color: #3b82f6;
    border-width: 2px;
    padding: 7px 11px;
}

QLineEdit:disabled, QTextEdit:disabled, QPlainTextEdit:disabled,
QSpinBox:disabled, QDoubleSpinBox:disabled, QComboBox:disabled {
    background-color: #0f172a;
    color: #64748b;
    border-color: #1e293b;
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
    border-top: 5px solid #94a3b8;
    width: 0;
    height: 0;
    margin-right: 8px;
}

QComboBox QAbstractItemView {
    background-color: #1e293b;
    border: 1px solid #334155;
    border-radius: 8px;
    padding: 4px;
    selection-background-color: #334155;
    selection-color: #f1f5f9;
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
    border: 2px solid #475569;
    border-radius: 4px;
    background-color: #1e293b;
}

QRadioButton::indicator {
    border-radius: 9px;
}

QCheckBox::indicator:hover, QRadioButton::indicator:hover {
    border-color: #3b82f6;
}

QCheckBox::indicator:checked, QRadioButton::indicator:checked {
    background-color: #3b82f6;
    border-color: #3b82f6;
}

QCheckBox:disabled, QRadioButton:disabled {
    color: #64748b;
}

QCheckBox::indicator:disabled, QRadioButton::indicator:disabled {
    border-color: #334155;
    background-color: #0f172a;
}

/* ========== Scrollbars ========== */
QScrollBar:vertical {
    background: transparent;
    width: 10px;
    margin: 2px;
    border-radius: 5px;
}

QScrollBar::handle:vertical {
    background: #475569;
    border-radius: 5px;
    min-height: 40px;
}

QScrollBar::handle:vertical:hover {
    background: #64748b;
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
    background: #475569;
    border-radius: 5px;
    min-width: 40px;
}

QScrollBar::handle:horizontal:hover {
    background: #64748b;
}

QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
    width: 0px;
}

QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
    background: none;
}

/* ========== Tab Widget ========== */
QTabWidget::pane {
    border: 1px solid #334155;
    border-radius: 0 8px 8px 8px;
    background-color: #1e293b;
    top: -1px;
}

QTabBar::tab {
    background-color: transparent;
    border: none;
    padding: 10px 20px;
    margin-right: 4px;
    color: #94a3b8;
    font-weight: 500;
    border-bottom: 2px solid transparent;
}

QTabBar::tab:hover {
    color: #f1f5f9;
    background-color: #334155;
    border-top-left-radius: 8px;
    border-top-right-radius: 8px;
}

QTabBar::tab:selected {
    color: #60a5fa;
    border-bottom: 2px solid #3b82f6;
    font-weight: 600;
}

/* ========== List & Tree Views ========== */
QListView, QTreeView, QTableView {
    background-color: #1e293b;
    border: 1px solid #334155;
    border-radius: 8px;
    padding: 4px;
    selection-background-color: #1e3a5f;
    selection-color: #93c5fd;
    outline: none;
}

QListView::item, QTreeView::item, QTableView::item {
    padding: 8px 12px;
    border-radius: 6px;
    margin: 2px;
}

QListView::item:hover, QTreeView::item:hover, QTableView::item:hover {
    background-color: #334155;
}

QListView::item:selected, QTreeView::item:selected, QTableView::item:selected {
    background-color: #1e3a5f;
    color: #93c5fd;
}

QHeaderView::section {
    background-color: #0f172a;
    border: none;
    border-bottom: 1px solid #334155;
    padding: 10px 12px;
    font-weight: 600;
    color: #cbd5e1;
}

/* ========== GroupBox ========== */
QGroupBox {
    border: 1px solid #334155;
    border-radius: 12px;
    margin-top: 16px;
    padding: 16px;
    background-color: #1e293b;
}

QGroupBox::title {
    subcontrol-origin: margin;
    left: 16px;
    padding: 0 8px;
    font-weight: 600;
    color: #f1f5f9;
}

/* ========== ToolTips ========== */
QToolTip {
    background-color: #334155;
    color: #f1f5f9;
    border: none;
    border-radius: 6px;
    padding: 8px 12px;
    font-size: 9pt;
}

/* ========== Menu & MenuBar ========== */
QMenuBar {
    background-color: #0f172a;
    border-bottom: 1px solid #334155;
    padding: 2px 8px;
}

QMenuBar::item {
    padding: 6px 12px;
    border-radius: 6px;
    background: transparent;
}

QMenuBar::item:selected {
    background-color: #334155;
}

QMenu {
    background-color: #1e293b;
    border: 1px solid #334155;
    border-radius: 8px;
    padding: 8px;
}

QMenu::item {
    padding: 8px 24px 8px 28px;
    border-radius: 6px;
    min-width: 180px;
}

QMenu::item:selected {
    background-color: #334155;
}

QMenu::separator {
    height: 1px;
    background: #334155;
    margin: 6px 8px;
}

/* ========== Progress Bar ========== */
QProgressBar {
    border: none;
    background-color: #334155;
    border-radius: 999px;
    height: 8px;
    text-align: center;
    color: transparent;
}

QProgressBar::chunk {
    background-color: #3b82f6;
    border-radius: 999px;
}

/* ========== Slider ========== */
QSlider::groove:horizontal {
    height: 4px;
    background: #334155;
    border-radius: 2px;
}

QSlider::handle:horizontal {
    background: #3b82f6;
    border: 2px solid #1e293b;
    width: 16px;
    height: 16px;
    margin: -6px 0;
    border-radius: 9px;
}

QSlider::handle:horizontal:hover {
    width: 20px;
    height: 20px;
    margin: -8px 0;
    border-radius: 10px;
}

QSlider::handle:horizontal:pressed {
    background: #2563eb;
    width: 18px;
    height: 18px;
    margin: -7px 0;
    border-radius: 9px;
}

/* ========== Status Bar ========== */
QStatusBar {
    background-color: #0f172a;
    border-top: 1px solid #334155;
    color: #94a3b8;
    padding: 4px 8px;
}

/* ========== ToolBar ========== */
QToolBar {
    background-color: #0f172a;
    border: none;
    border-bottom: 1px solid #334155;
    padding: 4px 8px;
    spacing: 4px;
}

QToolBar::separator {
    background-color: #334155;
    width: 1px;
    margin: 8px 8px;
}

QToolButton {
    background-color: transparent;
    border: 1px solid transparent;
    border-radius: 8px;
    padding: 6px 10px;
    color: #cbd5e1;
}

QToolButton:hover {
    background-color: #334155;
    border-color: #475569;
    color: #f1f5f9;
}

QToolButton:pressed {
    background-color: #1e293b;
}

QToolButton:checked {
    background-color: #1e3a5f;
    border-color: #3b82f6;
    color: #93c5fd;
}

QToolButton:disabled {
    color: #64748b;
}

QToolButton:focus {
    border-color: #3b82f6;
}

/* Primary CTA tool button */
QToolButton[objectName="primary"] {
    background-color: #3b82f6;
    border: 1px solid #3b82f6;
    color: #ffffff;
    font-weight: 600;
}

QToolButton[objectName="primary"]:hover {
    background-color: #2563eb;
    border-color: #2563eb;
    color: #ffffff;
}

QToolButton[objectName="primary"]:pressed {
    background-color: #1d4ed8;
    border-color: #1d4ed8;
}

QToolButton[objectName="primary"]:disabled {
    background-color: #475569;
    border-color: #475569;
    color: #94a3b8;
}

/* Vertical toolbar (instance sidebar) */
QToolBar[orientation="1"] {
    border-bottom: none;
    border-right: 1px solid #334155;
    padding: 12px 8px;
}

QToolBar[orientation="1"] QToolButton {
    padding: 10px 8px;
    min-width: 80px;
}

/* ========== Splitter ========== */
QSplitter::handle {
    background-color: #334155;
    width: 1px;
}

QSplitter::handle:hover {
    background-color: #3b82f6;
}

/* ========== Dock Widget ========== */
QDockWidget::title {
    background-color: #0f172a;
    border-bottom: 1px solid #334155;
    padding: 8px 12px;
    font-weight: 600;
}

/* ========== Tool Box ========== */
QToolBox::tab {
    background-color: #0f172a;
    border: 1px solid #334155;
    border-radius: 8px;
    padding: 12px;
    font-weight: 500;
}

QToolBox::tab:selected {
    background-color: #1e293b;
    border-color: #3b82f6;
    color: #60a5fa;
    font-weight: 600;
}

/* ========== Settings / Page Navigation ========== */
PageView {
    background-color: #0f172a;
    border: none;
    border-right: 1px solid #334155;
    padding: 8px 4px;
    outline: none;
}

PageView::item {
    border-radius: 8px;
    padding: 8px 12px;
    margin: 2px 4px;
    color: #cbd5e1;
}

PageView::item:hover {
    background-color: #334155;
    color: #f1f5f9;
}

PageView::item:selected {
    background-color: #1e3a5f;
    color: #93c5fd;
    font-weight: 500;
}

PageView::item:selected:active {
    background-color: #1e40af;
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
    background-color: #1e293b;
}

QScrollArea > QWidget > QWidget {
    background-color: #1e293b;
}

/* Settings group sections */
QGroupBox {
    margin-top: 16px;
    padding-top: 16px;
    padding-left: 16px;
    padding-right: 16px;
    padding-bottom: 12px;
    border-radius: 12px;
    border: 1px solid #334155;
    background-color: #0f172a;
    font-weight: 600;
    color: #f1f5f9;
}

QGroupBox::title {
    subcontrol-origin: margin;
    left: 16px;
    padding: 0 8px;
    color: #f1f5f9;
}

/* ========== Dialog Buttons ========== */
QDialogButtonBox {
    padding: 12px 16px;
    border-top: 1px solid #334155;
    background-color: #0f172a;
}

QDialogButtonBox QPushButton {
    min-width: 80px;
    padding: 6px 16px;
}

QDialogButtonBox QPushButton[dialogButtonCode="1"] {
    background-color: #3b82f6;
    color: white;
    border: 1px solid #3b82f6;
}

QDialogButtonBox QPushButton[dialogButtonCode="1"]:hover {
    background-color: #2563eb;
    border-color: #2563eb;
}

/* ========== Instance View Specific ========== */
InstanceView {
    background-color: #0f172a;
    border: none;
}

)";
}

QString ModernDarkTheme::tooltip()
{
    return QObject::tr("Modern dark UI design system with clean aesthetics and eye-friendly contrast");
}
