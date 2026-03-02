// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Requiem Mod Launcher - Minecraft Launcher
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
#include "RequiemTheme.h"

#include <QObject>

QString RequiemTheme::id()
{
    return "requiem";
}

QString RequiemTheme::name()
{
    return QObject::tr("Requiem");
}

QPalette RequiemTheme::colorScheme()
{
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(0x0d, 0x0d, 0x0f));
    palette.setColor(QPalette::WindowText, QColor(0xe0, 0xe0, 0xe8));
    palette.setColor(QPalette::Base, QColor(0x13, 0x13, 0x16));
    palette.setColor(QPalette::AlternateBase, QColor(0x1a, 0x1a, 0x1e));
    palette.setColor(QPalette::ToolTipBase, QColor(0x1e, 0x1e, 0x24));
    palette.setColor(QPalette::ToolTipText, QColor(0xe0, 0xe0, 0xe8));
    palette.setColor(QPalette::Text, QColor(0xe0, 0xe0, 0xe8));
    palette.setColor(QPalette::Button, QColor(0x13, 0x13, 0x16));
    palette.setColor(QPalette::ButtonText, QColor(0xe0, 0xe0, 0xe8));
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Link, QColor(0x3a, 0x3a, 0x44));
    palette.setColor(QPalette::Highlight, QColor(0x3a, 0x3a, 0x44));
    palette.setColor(QPalette::HighlightedText, QColor(0xe0, 0xe0, 0xe8));
    palette.setColor(QPalette::PlaceholderText, QColor(0x55, 0x55, 0x5e));
    return fadeInactive(palette, fadeAmount(), fadeColor());
}

double RequiemTheme::fadeAmount()
{
    return 0.5;
}

QColor RequiemTheme::fadeColor()
{
    return QColor(0x0d, 0x0d, 0x0f);
}

bool RequiemTheme::hasStyleSheet()
{
    return true;
}

QString RequiemTheme::appStyleSheet()
{
    return
        // Base widgets
        "QMainWindow, QWidget {"
        "  background-color: #0a0a0c;"
        "  color: #e0e0e8;"
        "}"

        // Menu bar
        "QMenuBar {"
        "  background-color: #0d0d0f;"
        "  color: #c8c8d0;"
        "  border-bottom: 1px solid #2a2a2e;"
        "}"
        "QMenuBar::item:selected {"
        "  background-color: #1e1e24;"
        "}"
        "QMenu {"
        "  background-color: #0d0d0f;"
        "  color: #c8c8d0;"
        "  border: 1px solid #2a2a2e;"
        "}"
        "QMenu::item:selected {"
        "  background-color: #1e1e24;"
        "}"

        // Toolbar
        "QToolBar {"
        "  background-color: #0d0d0f;"
        "  border-bottom: 1px solid #2a2a2e;"
        "}"

        // Buttons
        "QPushButton {"
        "  background-color: #131316;"
        "  border: 1px solid #2a2a2e;"
        "  color: #e0e0e8;"
        "  padding: 4px 8px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #1e1e24;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #1a1a1e;"
        "}"

        // Text inputs
        "QLineEdit, QTextEdit, QPlainTextEdit {"
        "  background-color: #131316;"
        "  border: 1px solid #2a2a2e;"
        "  color: #e0e0e8;"
        "  selection-background-color: #1e1e24;"
        "}"

        // Item views
        "QListView, QTreeView, QTableView {"
        "  background-color: #0d0d0f;"
        "  alternate-background-color: #131316;"
        "  color: #e0e0e8;"
        "  selection-background-color: #1e1e24;"
        "  gridline-color: #2a2a2e;"
        "  border: 1px solid #2a2a2e;"
        "}"
        "QListView::item:selected, QTreeView::item:selected, QTableView::item:selected {"
        "  background-color: #1e1e24;"
        "  color: #e0e0e8;"
        "}"

        // Header view
        "QHeaderView::section {"
        "  background-color: #131316;"
        "  border: 1px solid #2a2a2e;"
        "  color: #c8c8d0;"
        "  padding: 2px 4px;"
        "}"

        // Tab widget
        "QTabWidget::pane {"
        "  border: 1px solid #2a2a2e;"
        "  background-color: #0a0a0c;"
        "}"
        "QTabBar::tab {"
        "  background-color: #131316;"
        "  color: #55555e;"
        "  padding: 4px 8px;"
        "  border: 1px solid #2a2a2e;"
        "}"
        "QTabBar::tab:selected {"
        "  background-color: #1e1e24;"
        "  color: #e0e0e8;"
        "}"
        "QTabBar::tab:hover:!selected {"
        "  background-color: #1a1a1e;"
        "}"

        // Scrollbars
        "QScrollBar:vertical, QScrollBar:horizontal {"
        "  background-color: #0a0a0c;"
        "  border: none;"
        "  width: 10px;"
        "  height: 10px;"
        "}"
        "QScrollBar::handle:vertical, QScrollBar::handle:horizontal {"
        "  background-color: #2a2a2e;"
        "  border-radius: 5px;"
        "  min-height: 20px;"
        "  min-width: 20px;"
        "}"
        "QScrollBar::handle:vertical:hover, QScrollBar::handle:horizontal:hover {"
        "  background-color: #3a3a44;"
        "}"
        "QScrollBar::add-line, QScrollBar::sub-line {"
        "  background: none;"
        "  border: none;"
        "}"

        // Group box
        "QGroupBox {"
        "  border: 1px solid #2a2a2e;"
        "  color: #c8c8d0;"
        "  margin-top: 6px;"
        "  padding-top: 6px;"
        "}"
        "QGroupBox::title {"
        "  color: #c8c8d0;"
        "  subcontrol-origin: margin;"
        "  left: 8px;"
        "}"

        // Combo box
        "QComboBox {"
        "  background-color: #131316;"
        "  border: 1px solid #2a2a2e;"
        "  color: #e0e0e8;"
        "  padding: 2px 4px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color: #131316;"
        "  border: 1px solid #2a2a2e;"
        "  color: #e0e0e8;"
        "  selection-background-color: #1e1e24;"
        "}"

        // Checkboxes and radio buttons
        "QCheckBox, QRadioButton {"
        "  color: #e0e0e8;"
        "}"

        // Progress bar
        "QProgressBar {"
        "  background-color: #131316;"
        "  border: 1px solid #2a2a2e;"
        "  color: #e0e0e8;"
        "  text-align: center;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: #3a3a44;"
        "}"

        // Status bar
        "QStatusBar {"
        "  background-color: #0d0d0f;"
        "  color: #55555e;"
        "}"

        // Tooltip
        "QToolTip {"
        "  background-color: #1e1e24;"
        "  color: #e0e0e8;"
        "  border: 1px solid #2a2a2e;"
        "}"

        // Dialog
        "QDialog {"
        "  background-color: #0a0a0c;"
        "}"

        // Splitter
        "QSplitter::handle {"
        "  background-color: #2a2a2e;"
        "}"

        // Spin box
        "QSpinBox, QDoubleSpinBox {"
        "  background-color: #131316;"
        "  border: 1px solid #2a2a2e;"
        "  color: #e0e0e8;"
        "}";
}

QString RequiemTheme::tooltip()
{
    return "";
}
