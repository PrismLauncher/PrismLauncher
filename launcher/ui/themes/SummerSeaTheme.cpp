// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 */
#include "SummerSeaTheme.h"

#include <QObject>

QString SummerSeaTheme::id()
{
    return "summer_sea";
}

QString SummerSeaTheme::name()
{
    return QObject::tr("Summer Sea");
}

QString SummerSeaTheme::tooltip()
{
    return QObject::tr("A beautiful sea & summer themed style with ocean gradients and cyan accents.");
}

bool SummerSeaTheme::hasStyleSheet()
{
    return true;
}

QString SummerSeaTheme::appStyleSheet()
{
    return 
        "QMainWindow, QDialog {"
        "    background-color: #0c2e4d;"
        "}"
        "QWidget {"
        "    background-color: #0c2e4d;"
        "}"
        "QLabel {"
        "    color: #e0f7ff;"
        "}"
        "QToolTip {"
        "    color: #ffffff;"
        "    background-color: #0a1f3e;"
        "    border: 2px solid #00d9ff;"
        "    border-radius: 6px;"
        "    padding: 6px;"
        "    font-weight: 500;"
        "}"
        "QLineEdit, QTextEdit, QPlainTextEdit, QSpinBox, QDoubleSpinBox, QComboBox {"
        "    background-color: #1a4a6f;"
        "    color: #e0f7ff;"
        "    border: 2px solid #0088bb;"
        "    border-radius: 8px;"
        "    padding: 6px 10px;"
        "    selection-background-color: #00b4d8;"
        "    selection-color: #0c2e4d;"
        "}"
        "QLineEdit:hover, QSpinBox:hover, QComboBox:hover {"
        "    border: 2px solid #00d9ff;"
        "    background-color: #215a7f;"
        "}"
        "QLineEdit:focus, QSpinBox:focus, QComboBox:focus {"
        "    border: 2px solid #00f5ff;"
        "    background-color: #2a6a8f;"
        "    box-shadow: 0 0 8px rgba(0, 212, 255, 0.3);"
        "}"
        "QMainWindow::separator {"
        "    background-color: #0a1f3e;"
        "    width: 2px;"
        "    height: 2px;"
        "}"
        "QToolBar {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1a4a6f, stop:1 #0c2e4d);"
        "    border: none;"
        "    border-bottom: 2px solid #0088bb;"
        "    padding: 4px;"
        "    spacing: 2px;"
        "}"
        "QToolBar::handle {"
        "    background-color: #0088bb;"
        "    width: 4px;"
        "    border-radius: 2px;"
        "}"
        "QMenuBar {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1a4a6f, stop:1 #0c2e4d);"
        "    color: #e0f7ff;"
        "    border-bottom: 2px solid #0088bb;"
        "    padding: 2px;"
        "}"
        "QMenuBar::item:selected {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #00a8d8, stop:1 #0088bb);"
        "    color: #0c2e4d;"
        "    border-radius: 4px;"
        "    padding: 4px 10px;"
        "}"
        "QMenuBar::item:pressed {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #0088bb, stop:1 #00a8d8);"
        "}"
        "QPushButton, QToolButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1f5a7f, stop:1 #0f3a5f);"
        "    color: #e0f7ff;"
        "    border: 2px solid #00a8d8;"
        "    border-radius: 8px;"
        "    padding: 8px 16px;"
        "    font-weight: bold;"
        "    font-size: 11px;"
        "}"
        "QPushButton:hover, QToolButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #2a7a9f, stop:1 #1a5a7f);"
        "    border: 2px solid #00d9ff;"
        "}"
        "QPushButton:pressed, QToolButton:pressed {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #0f3a5f, stop:1 #1f5a7f);"
        "    border: 2px solid #00f5ff;"
        "}"
        "QPushButton:disabled, QToolButton:disabled {"
        "    background-color: #0a2040;"
        "    color: #4a7a9f;"
        "    border: 2px solid #0a4070;"
        "}"
        "QTabWidget::pane {"
        "    border: 2px solid #0088bb;"
        "    border-radius: 10px;"
        "    background-color: #0c2e4d;"
        "}"
        "QTabBar::tab {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1a4a6f, stop:1 #0f3a5f);"
        "    color: #a0d0e8;"
        "    border: 2px solid #0088bb;"
        "    border-bottom: none;"
        "    border-top-left-radius: 8px;"
        "    border-top-right-radius: 8px;"
        "    padding: 10px 20px;"
        "    margin-right: 2px;"
        "    font-weight: 500;"
        "}"
        "QTabBar::tab:selected {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #00a8d8, stop:1 #0088bb);"
        "    color: #0c2e4d;"
        "    border-bottom: 3px solid #00f5ff;"
        "    font-weight: bold;"
        "}"
        "QTabBar::tab:hover:!selected {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #2a6a8f, stop:1 #1a4a6f);"
        "    color: #ffffff;"
        "}"
        "QScrollBar:vertical {"
        "    background-color: #0a1f3e;"
        "    width: 12px;"
        "    margin: 0px;"
        "    border-radius: 6px;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00a8d8, stop:1 #0088bb);"
        "    min-height: 24px;"
        "    border-radius: 6px;"
        "    margin: 2px 2px 2px 2px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00d9ff, stop:1 #00b4d8);"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "    height: 0px;"
        "}"
        "QScrollBar:horizontal {"
        "    background-color: #0a1f3e;"
        "    height: 12px;"
        "    margin: 0px;"
        "    border-radius: 6px;"
        "}"
        "QScrollBar::handle:horizontal {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #00a8d8, stop:1 #0088bb);"
        "    min-width: 24px;"
        "    border-radius: 6px;"
        "    margin: 2px 2px 2px 2px;"
        "}"
        "QScrollBar::handle:horizontal:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #00d9ff, stop:1 #00b4d8);"
        "}"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
        "    width: 0px;"
        "}"
        "QMenu {"
        "    background-color: #0c2e4d;"
        "    color: #e0f7ff;"
        "    border: 2px solid #0088bb;"
        "    border-radius: 8px;"
        "}"
        "QMenu::item:selected {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #00a8d8, stop:1 #0088bb);"
        "    color: #0c2e4d;"
        "    border-radius: 4px;"
        "}"
        "QHeaderView::section {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1a4a6f, stop:1 #0f3a5f);"
        "    color: #e0f7ff;"
        "    padding: 8px;"
        "    border: 2px solid #0088bb;"
        "    font-weight: bold;"
        "}"
        "QListView, QTreeView, QTableView {"
        "    background-color: rgba(10, 30, 60, 0.8);"
        "    border: 2px solid #0088bb;"
        "    border-radius: 8px;"
        "    color: #e0f7ff;"
        "    gridline-color: #0a5080;"
        "}"
        "QListView::item:selected, QTreeView::item:selected, QTableView::item:selected {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00a8d8, stop:1 #0088bb);"
        "    color: #0c2e4d;"
        "}"
        "QGroupBox {"
        "    border: 2px solid #0088bb;"
        "    border-radius: 10px;"
        "    margin-top: 1.5em;"
        "    padding-top: 1em;"
        "    font-weight: bold;"
        "    color: #00d9ff;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    subcontrol-position: top left;"
        "    padding: 0 8px;"
        "    left: 12px;"
        "}"
        "QSlider::groove:horizontal {"
        "    background-color: #1a4a6f;"
        "    border-radius: 5px;"
        "    height: 8px;"
        "}"
        "QSlider::handle:horizontal {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #00d9ff, stop:1 #0088bb);"
        "    width: 18px;"
        "    margin: -5px 0;"
        "    border-radius: 9px;"
        "    border: 2px solid #00b4d8;"
        "}"
        "QSlider::handle:horizontal:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #00f5ff, stop:1 #00d9ff);"
        "    border: 2px solid #00f5ff;"
        "}"
        "QCheckBox {"
        "    color: #e0f7ff;"
        "    spacing: 8px;"
        "}"
        "QCheckBox::indicator {"
        "    width: 18px;"
        "    height: 18px;"
        "    border-radius: 4px;"
        "}"
        "QCheckBox::indicator:unchecked {"
        "    background-color: #1a4a6f;"
        "    border: 2px solid #0088bb;"
        "}"
        "QCheckBox::indicator:checked {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #00d9ff, stop:1 #0088bb);"
        "    border: 2px solid #00d9ff;"
        "    image: url(:/data/icons/breeze/check.svg);"
        "}"
        "QRadioButton {"
        "    color: #e0f7ff;"
        "    spacing: 8px;"
        "}"
        "QRadioButton::indicator {"
        "    width: 18px;"
        "    height: 18px;"
        "}"
        "QRadioButton::indicator:unchecked {"
        "    background-color: #1a4a6f;"
        "    border: 2px solid #0088bb;"
        "    border-radius: 9px;"
        "}"
        "QRadioButton::indicator:checked {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #00d9ff, stop:1 #0088bb);"
        "    border: 2px solid #00d9ff;"
        "    border-radius: 9px;"
        "}";
}

QPalette SummerSeaTheme::colorScheme()
{
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(12, 46, 77));
    palette.setColor(QPalette::WindowText, QColor(224, 247, 255));
    palette.setColor(QPalette::Base, QColor(26, 74, 111));
    palette.setColor(QPalette::AlternateBase, QColor(20, 60, 95));
    palette.setColor(QPalette::ToolTipBase, QColor(10, 31, 62));
    palette.setColor(QPalette::ToolTipText, QColor(224, 247, 255));
    palette.setColor(QPalette::Text, QColor(224, 247, 255));
    palette.setColor(QPalette::Button, QColor(26, 74, 111));
    palette.setColor(QPalette::ButtonText, QColor(224, 247, 255));
    palette.setColor(QPalette::BrightText, QColor(0, 212, 255));
    palette.setColor(QPalette::Link, QColor(0, 168, 216));
    palette.setColor(QPalette::Highlight, QColor(0, 168, 216));
    palette.setColor(QPalette::HighlightedText, QColor(12, 46, 77));
    palette.setColor(QPalette::PlaceholderText, QColor(128, 160, 192));
    return fadeInactive(palette, fadeAmount(), fadeColor());
}

double SummerSeaTheme::fadeAmount()
{
    return 0.5;
}

QColor SummerSeaTheme::fadeColor()
{
    return QColor(12, 46, 77);
}
