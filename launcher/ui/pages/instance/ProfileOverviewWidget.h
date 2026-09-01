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

#pragma once

#include <QWidget>
#include <QSet>
#include <QString>
#include <QMap>
#include <QStringList>
#include <QLayout>
#include <QStyledItemDelegate>
#include <optional>

class ModFolderModel;
class QTableWidget;
class QLabel;
class QPushButton;
class QFrame;

class FlowLayout : public QLayout {
   public:
    explicit FlowLayout(QWidget* parent, int margin = 0, int hSpacing = 6, int vSpacing = 6);
    ~FlowLayout() override;

    void addItem(QLayoutItem* item) override;
    int horizontalSpacing() const;
    int verticalSpacing() const;
    Qt::Orientations expandingDirections() const override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int) const override;
    int count() const override;
    QLayoutItem* itemAt(int index) const override;
    QSize minimumSize() const override;
    void setGeometry(const QRect& rect) override;
    QSize sizeHint() const override;
    QLayoutItem* takeAt(int index) override;

   private:
    int doLayout(const QRect& rect, bool testOnly) const;

    QList<QLayoutItem*> m_itemList;
    int m_hSpace;
    int m_vSpace;
};

class OverviewItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
   public:
    explicit OverviewItemDelegate(QObject* parent = nullptr);

    void initStyleOption(QStyleOptionViewItem* option,
                         const QModelIndex& index) const override;

    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;
};

class ProfileOverviewWidget : public QWidget {
    Q_OBJECT
   public:
    explicit ProfileOverviewWidget(QWidget* parent = nullptr);

    void refresh(ModFolderModel* model,
                 const QMap<QString, QSet<QString>>& profileStates,
                 const QStringList& profileNames,
                 bool overviewDefault,
                 const QStringList& selectedProfiles,
                 bool isRunning,
                 const std::optional<QSet<QString>>& launchSnapshot);

    void setRunningLocked(bool running);

    void filterTextChanged(const QString& filter);

    void setOverviewActive(bool active);

   protected:
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

   signals:
    void defaultSelected();
    void profileSelectionToggled(const QString& profileName, bool selected);
    void overviewExitRequested();

   private:
    void buildSelectionRow(const QStringList& profileNames,
                           bool overviewDefault,
                           const QStringList& selectedProfiles,
                           bool isRunning);

    QWidget*      m_selectionRow       = nullptr;
    FlowLayout*   m_selectionLayout    = nullptr;
    QPushButton*  m_defaultButton      = nullptr;
    QFrame*       m_separator          = nullptr;
    QPushButton*  m_overviewExitButton = nullptr;
    QList<QPushButton*> m_profileButtons;

    QTableWidget* m_table       = nullptr;
    QLabel*       m_statusLabel = nullptr;
};
