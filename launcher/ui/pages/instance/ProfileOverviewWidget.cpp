// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2022 TheKodeToad <TheKodeToad@proton.me>
 *  Copyright (c) 2023 Trial97 <alexandru.tripon97@gmail.com>
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

#include "ProfileOverviewWidget.h"

#include <QFrame>
#include <QHeaderView>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QStyle>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "minecraft/mod/ModFolderModel.h"

FlowLayout::FlowLayout(QWidget* parent, int margin, int hSpacing, int vSpacing)
    : QLayout(parent), m_hSpace(hSpacing), m_vSpace(vSpacing)
{
    setContentsMargins(margin, margin, margin, margin);
}

FlowLayout::~FlowLayout()
{
    QLayoutItem* item;
    while ((item = takeAt(0)))
        delete item;
}

void FlowLayout::addItem(QLayoutItem* item)
{
    m_itemList.append(item);
}

int FlowLayout::horizontalSpacing() const
{
    return m_hSpace >= 0 ? m_hSpace : 4;
}

int FlowLayout::verticalSpacing() const
{
    return m_vSpace >= 0 ? m_vSpace : 4;
}

int FlowLayout::count() const
{
    return m_itemList.size();
}

QLayoutItem* FlowLayout::itemAt(int index) const
{
    return m_itemList.value(index);
}

QLayoutItem* FlowLayout::takeAt(int index)
{
    if (index >= 0 && index < m_itemList.size())
        return m_itemList.takeAt(index);
    return nullptr;
}

Qt::Orientations FlowLayout::expandingDirections() const
{
    return {};
}

bool FlowLayout::hasHeightForWidth() const
{
    return true;
}

int FlowLayout::heightForWidth(int width) const
{
    return doLayout(QRect(0, 0, width, 0), true);
}

void FlowLayout::setGeometry(const QRect& rect)
{
    QLayout::setGeometry(rect);
    doLayout(rect, false);
}

QSize FlowLayout::sizeHint() const
{
    return minimumSize();
}

QSize FlowLayout::minimumSize() const
{
    QSize size;
    for (const QLayoutItem* item : m_itemList)
        size = size.expandedTo(item->minimumSize());
    int left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);
    return size + QSize(left + right, top + bottom);
}

int FlowLayout::doLayout(const QRect& rect, bool testOnly) const
{
    int left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);
    QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);
    int x = effectiveRect.x();
    int y = effectiveRect.y();
    int lineHeight = 0;

    for (QLayoutItem* item : m_itemList) {
        QWidget* wid = item->widget();
        if (wid && wid->isHidden())
            continue;

        int spaceX = horizontalSpacing();
        int spaceY = verticalSpacing();
        int itemWidth = item->sizeHint().width();
        int itemHeight = item->sizeHint().height();

        int nextX = x + itemWidth + spaceX;
        if (nextX - spaceX > effectiveRect.right() && lineHeight > 0) {
            x = effectiveRect.x();
            y = y + lineHeight + spaceY;
            nextX = x + itemWidth + spaceX;
            lineHeight = 0;
        }

        if (!testOnly)
            item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));

        x = nextX;
        lineHeight = qMax(lineHeight, itemHeight);
    }
    return y + lineHeight - rect.y() + bottom;
}

OverviewItemDelegate::OverviewItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{}

void OverviewItemDelegate::initStyleOption(QStyleOptionViewItem* option,
                                          const QModelIndex& index) const
{
    QStyledItemDelegate::initStyleOption(option, index);
    if (index.column() == 2 || index.column() == 3) {
        option->text.clear();
        option->icon = QIcon();
    }
}

void OverviewItemDelegate::paint(QPainter* painter,
                                const QStyleOptionViewItem& option,
                                const QModelIndex& index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const bool isDark = option.palette.color(QPalette::Base).lightness() < 128;

    if (index.column() == 0) {
        QStyleOptionViewItem opt = option;
        opt.features |= QStyleOptionViewItem::HasCheckIndicator;
        opt.checkState = index.data(Qt::CheckStateRole).value<Qt::CheckState>();
        opt.state |= QStyle::State_Enabled;
        if (opt.checkState == Qt::Checked) {
            opt.state |= QStyle::State_On;
            opt.state &= ~QStyle::State_Off;
        } else {
            opt.state |= QStyle::State_Off;
            opt.state &= ~QStyle::State_On;
        }
        QStyledItemDelegate::paint(painter, opt, index);
        painter->restore();
        return;
    }

    if (index.column() == 2) {
        QStyledItemDelegate::paint(painter, option, index);

        QString statusText = index.data(Qt::DisplayRole).toString();
        if (!statusText.isEmpty()) {
            QColor bg, border, text;
            if (statusText == tr("Enabled")) {
                bg = isDark ? QColor(35, 75, 45) : QColor(225, 248, 230);
                border = isDark ? QColor(55, 130, 75) : QColor(140, 210, 155);
                text = isDark ? QColor(145, 235, 165) : QColor(20, 115, 45);
            } else if (statusText == tr("Disabled")) {
                bg = isDark ? QColor(50, 50, 50) : QColor(235, 235, 235);
                border = isDark ? QColor(90, 90, 90) : QColor(190, 190, 190);
                text = isDark ? QColor(170, 170, 170) : QColor(90, 90, 90);
            } else {
                bg = isDark ? QColor(80, 55, 15) : QColor(255, 244, 215);
                border = isDark ? QColor(160, 110, 30) : QColor(235, 180, 70);
                text = isDark ? QColor(255, 205, 100) : QColor(150, 85, 0);
            }

            QFont font = option.font;
            font.setPointSize(qMax(font.pointSize() - 1, 8));
            QFontMetrics fm(font);
            int textWidth = fm.horizontalAdvance(statusText);
            int chipWidth = textWidth + 16;
            int chipHeight = 20;
            int chipX = option.rect.left() + 4;
            int chipY = option.rect.top() + (option.rect.height() - chipHeight) / 2;
            QRect chipRect(chipX, chipY, chipWidth, chipHeight);

            painter->setPen(QPen(border, 1));
            painter->setBrush(bg);
            painter->drawRoundedRect(chipRect, 4, 4);

            painter->setFont(font);
            painter->setPen(text);
            painter->drawText(chipRect, Qt::AlignCenter, statusText);
        }
        painter->restore();
        return;
    }

    if (index.column() == 3) {
        QStyledItemDelegate::paint(painter, option, index);

        QStringList sources = index.data(Qt::UserRole).toStringList();
        if (sources.isEmpty()) {
            QFont font = option.font;
            painter->setFont(font);
            painter->setPen(option.palette.color(QPalette::PlaceholderText));
            painter->drawText(option.rect.adjusted(6, 0, -6, 0), Qt::AlignVCenter | Qt::AlignLeft, QStringLiteral("—"));
        } else {
            QFont font = option.font;
            font.setPointSize(qMax(font.pointSize() - 1, 8));
            painter->setFont(font);
            QFontMetrics fm(font);

            QColor chipBg = option.palette.color(QPalette::Button);
            QColor chipBorder = isDark ? QColor(80, 80, 80) : QColor(190, 190, 190);
            QColor chipText = option.palette.color(QPalette::ButtonText);

            int startX = option.rect.left() + 4;
            int startY = option.rect.top() + 3;
            int curX = startX;
            int curY = startY;
            int chipHeight = 20;
            int hSpacing = 5;
            int vSpacing = 3;
            int rightLimit = option.rect.right() - 4;

            for (const QString& name : sources) {
                int textWidth = fm.horizontalAdvance(name);
                int chipWidth = textWidth + 14;

                if (curX + chipWidth > rightLimit && curX > startX) {
                    curX = startX;
                    curY += chipHeight + vSpacing;
                }

                int actualWidth = qMin(chipWidth, rightLimit - curX);
                if (actualWidth < 10)
                    break;

                QRect chipRect(curX, curY, actualWidth, chipHeight);
                painter->setPen(QPen(chipBorder, 1));
                painter->setBrush(chipBg);
                painter->drawRoundedRect(chipRect, 4, 4);

                painter->setPen(chipText);
                painter->drawText(chipRect, Qt::AlignCenter, fm.elidedText(name, Qt::ElideRight, chipRect.width() - 6));

                curX += chipWidth + hSpacing;
            }
        }
        painter->restore();
        return;
    }

    painter->restore();
    QStyledItemDelegate::paint(painter, option, index);
}

QSize OverviewItemDelegate::sizeHint(const QStyleOptionViewItem& option,
                                    const QModelIndex& index) const
{
    if (index.column() == 0) {
        return QSize(36, 26);
    }
    if (index.column() == 2) {
        QString statusText = index.data(Qt::DisplayRole).toString();
        QFont font = option.font;
        font.setPointSize(qMax(font.pointSize() - 1, 8));
        QFontMetrics fm(font);
        return QSize(fm.horizontalAdvance(statusText) + 24, 26);
    }
    if (index.column() == 3) {
        QStringList sources = index.data(Qt::UserRole).toStringList();
        if (sources.isEmpty())
            return QSize(40, 26);

        QFont font = option.font;
        font.setPointSize(qMax(font.pointSize() - 1, 8));
        QFontMetrics fm(font);

        int colWidth = option.rect.width();
        if (const auto* view = qobject_cast<const QTableView*>(option.widget)) {
            int w = view->columnWidth(index.column());
            if (w > 0)
                colWidth = w;
        }

        int availableWidth = colWidth > 20 ? (colWidth - 8) : 200;
        int startX = 0;
        int curX = startX;
        int numLines = 1;
        int hSpacing = 5;

        for (const QString& name : sources) {
            int chipWidth = fm.horizontalAdvance(name) + 14;
            if (curX + chipWidth > availableWidth && curX > startX) {
                curX = startX;
                numLines++;
            }
            curX += chipWidth + hSpacing;
        }

        int height = numLines * 20 + (numLines - 1) * 3 + 6;
        return QSize(availableWidth, qMax(height, 26));
    }
    return QStyledItemDelegate::sizeHint(option, index);
}

ProfileOverviewWidget::ProfileOverviewWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 2, 0, 0);
    mainLayout->setSpacing(4);

    auto* headerRow = new QWidget(this);
    auto* headerLayout = new QHBoxLayout(headerRow);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(8);

    m_selectionRow = new QWidget(headerRow);
    m_selectionLayout = new FlowLayout(m_selectionRow, 0, 4, 4);

    m_defaultButton = new QPushButton(tr("Default"), m_selectionRow);
    m_defaultButton->setCheckable(true);
    QFont defFont = m_defaultButton->font();
    defFont.setBold(true);
    m_defaultButton->setFont(defFont);
    m_defaultButton->setToolTip(tr("Use Default's mod set exclusively at launch."));
    connect(m_defaultButton, &QPushButton::clicked, this, [this](bool) {
        emit defaultSelected();
    });
    m_selectionLayout->addWidget(m_defaultButton);

    m_separator = new QFrame(m_selectionRow);
    m_separator->setFrameShape(QFrame::VLine);
    m_separator->setFrameShadow(QFrame::Sunken);
    m_separator->setFixedSize(2, 22);
    m_selectionLayout->addWidget(m_separator);

    headerLayout->addWidget(m_selectionRow, 1);

    m_overviewExitButton = new QPushButton(tr("Profile Overview"), headerRow);
    m_overviewExitButton->setCheckable(true);
    m_overviewExitButton->setChecked(true);
    m_overviewExitButton->setIcon(QIcon::fromTheme("loadermods"));
    m_overviewExitButton->setIconSize(QSize(16, 16));
    m_overviewExitButton->setToolTip(
        tr("Show the read-only launch composition overview.\n"
           "Select which profiles are combined when launching the game."));
    connect(m_overviewExitButton, &QPushButton::clicked, this, &ProfileOverviewWidget::overviewExitRequested);
    headerLayout->addWidget(m_overviewExitButton, 0, Qt::AlignTop | Qt::AlignRight);

    mainLayout->addWidget(headerRow);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setContentsMargins(4, 0, 4, 0);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setVisible(false);
    mainLayout->addWidget(m_statusLabel);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({ tr("Enabled at Launch"), tr("Mod"), tr("Status"), tr("Source Profiles") });
    m_table->setItemDelegate(new OverviewItemDelegate(this));
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(26);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);
    connect(m_table->horizontalHeader(), &QHeaderView::sectionResized, m_table, &QTableWidget::resizeRowsToContents);
    connect(m_table->horizontalHeader(), &QHeaderView::geometriesChanged, m_table, &QTableWidget::resizeRowsToContents);
    mainLayout->addWidget(m_table, 1);
}

void ProfileOverviewWidget::buildSelectionRow(const QStringList& profileNames,
                                              bool overviewDefault,
                                              const QStringList& selectedProfiles,
                                              bool isRunning)
{
    for (auto* btn : m_profileButtons) {
        m_selectionLayout->removeWidget(btn);
        delete btn;
    }
    m_profileButtons.clear();

    m_defaultButton->setChecked(overviewDefault);
    m_defaultButton->setEnabled(!isRunning);

    for (int i = 1; i < profileNames.size(); ++i) {
        const QString& name = profileNames.at(i);
        auto* btn = new QPushButton(name, m_selectionRow);
        btn->setCheckable(true);
        btn->setChecked(!overviewDefault && selectedProfiles.contains(name));
        btn->setEnabled(!isRunning);
        if (isRunning)
            btn->setToolTip(tr("Cannot change launch selection while Minecraft is running."));
        else
            btn->setToolTip(tr("Toggle this profile in the launch composition."));
        connect(btn, &QPushButton::clicked, this, [this, name](bool checked) {
            emit profileSelectionToggled(name, checked);
        });
        m_selectionLayout->addWidget(btn);
        m_profileButtons.append(btn);
    }
    m_selectionRow->updateGeometry();
}

void ProfileOverviewWidget::setRunningLocked(bool running)
{
    m_defaultButton->setEnabled(!running);
    for (auto* btn : m_profileButtons)
        btn->setEnabled(!running);
    if (running) {
        m_statusLabel->setText(tr("Minecraft is running — launch composition is locked."));
        m_statusLabel->setVisible(true);
    } else {
        m_statusLabel->clear();
        m_statusLabel->setVisible(false);
    }
}

void ProfileOverviewWidget::refresh(ModFolderModel* model,
                                    const QMap<QString, QSet<QString>>& profileStates,
                                    const QStringList& profileNames,
                                    bool overviewDefault,
                                    const QStringList& selectedProfiles,
                                    bool isRunning,
                                    const std::optional<QSet<QString>>& launchSnapshot)
{
    buildSelectionRow(profileNames, overviewDefault, selectedProfiles, isRunning);

    if (m_overviewExitButton)
        m_overviewExitButton->setChecked(true);

    if (isRunning) {
        m_statusLabel->setText(tr("Minecraft is running — launch composition is locked."));
        m_statusLabel->setVisible(true);
    } else {
        m_statusLabel->clear();
        m_statusLabel->setVisible(false);
    }

    QSet<QString> nextLaunchIds;
    if (overviewDefault) {
        if (!profileNames.isEmpty())
            nextLaunchIds = profileStates.value(profileNames.at(0));
    } else {
        for (const QString& pname : selectedProfiles)
            nextLaunchIds += profileStates.value(pname);
    }

    QMap<QString, QString> modIdToName;
    for (int i = 0; i < model->rowCount(); ++i) {
        const auto& mod = model->at(i);
        modIdToName[mod.mod_id()] = mod.name();
    }

    QSet<QString> displayIds = nextLaunchIds;
    if (isRunning && launchSnapshot.has_value())
        displayIds += launchSnapshot.value();

    m_table->setRowCount(0);
    m_table->setSortingEnabled(false);

    for (const QString& modId : std::as_const(displayIds)) {
        const bool inNextLaunch = nextLaunchIds.contains(modId);
        const bool inSnapshot   = isRunning && launchSnapshot.has_value() && launchSnapshot->contains(modId);
        const bool launchEnabled = isRunning ? inSnapshot : inNextLaunch;

        QString statusText;
        if (isRunning && launchSnapshot.has_value()) {
            bool nextLaunchEnabled = inNextLaunch;
            if (inSnapshot == nextLaunchEnabled)
                statusText = inSnapshot ? tr("Enabled") : tr("Disabled");
            else if (inSnapshot && !nextLaunchEnabled)
                statusText = tr("Override · Disabled next launch");
            else
                statusText = tr("Override · Enabled next launch");
        } else {
            statusText = inNextLaunch ? tr("Enabled") : tr("Disabled");
        }

        QStringList sources;
        if (overviewDefault) {
            if (!profileNames.isEmpty() && profileStates.value(profileNames.at(0)).contains(modId))
                sources.append(profileNames.at(0));
        } else {
            for (const QString& pname : selectedProfiles) {
                if (profileStates.value(pname).contains(modId))
                    sources.append(pname);
            }
        }

        int row = m_table->rowCount();
        m_table->insertRow(row);

        auto* checkItem = new QTableWidgetItem();
        checkItem->setCheckState(launchEnabled ? Qt::Checked : Qt::Unchecked);
        checkItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        m_table->setItem(row, 0, checkItem);

        QString displayName = modIdToName.value(modId, modId);
        auto* nameItem = new QTableWidgetItem(displayName);
        nameItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        m_table->setItem(row, 1, nameItem);

        auto* statusItem = new QTableWidgetItem(statusText);
        statusItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        m_table->setItem(row, 2, statusItem);

        auto* sourceItem = new QTableWidgetItem();
        sourceItem->setData(Qt::UserRole, sources);
        sourceItem->setData(Qt::DisplayRole, sources.join(QStringLiteral(", ")));
        sourceItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        m_table->setItem(row, 3, sourceItem);
    }

    m_table->setSortingEnabled(true);
    m_table->sortByColumn(1, Qt::AscendingOrder);
    m_table->resizeRowsToContents();
}

void ProfileOverviewWidget::filterTextChanged(const QString& filter)
{
    if (!m_table)
        return;
    for (int row = 0; row < m_table->rowCount(); ++row) {
        if (filter.isEmpty()) {
            m_table->setRowHidden(row, false);
            continue;
        }
        auto* nameItem = m_table->item(row, 1);
        auto* statusItem = m_table->item(row, 2);
        auto* sourceItem = m_table->item(row, 3);
        bool match = (nameItem && nameItem->text().contains(filter, Qt::CaseInsensitive)) ||
                     (statusItem && statusItem->text().contains(filter, Qt::CaseInsensitive)) ||
                     (sourceItem && sourceItem->text().contains(filter, Qt::CaseInsensitive));
        m_table->setRowHidden(row, !match);
    }
    m_table->resizeRowsToContents();
}

void ProfileOverviewWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    if (m_overviewExitButton)
        m_overviewExitButton->setChecked(true);
    if (m_table)
        m_table->resizeRowsToContents();
}

void ProfileOverviewWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (m_table)
        m_table->resizeRowsToContents();
}

void ProfileOverviewWidget::setOverviewActive(bool active)
{
    if (m_overviewExitButton)
        m_overviewExitButton->setChecked(active);
}
