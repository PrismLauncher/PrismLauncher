// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
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

#include <QListView>
#include <QModelIndex>
#include <QWidget>

#include "ui/pages/BasePageContainer.h"
#include "ui/pages/BasePageProvider.h"

class QLayout;
class IconLabel;
class QSortFilterProxyModel;
class PageModel;
class QLabel;
class QListView;
class QLineEdit;
class QStackedLayout;
class QGridLayout;

class PageContainer : public QWidget, public BasePageContainer {
    Q_OBJECT
   public:
    explicit PageContainer(BasePageProvider* pageProvider, QString defaultId = QString(), QWidget* parent = nullptr);
    ~PageContainer() override = default;

    void addButtons(QWidget* buttons);
    void addButtons(QLayout* buttons);

    void useSidebarStyle(bool sidebar);

    /*
     * Save any unsaved state and prepare to be closed.
     * @return true if everything can be saved, false if there is something that requires attention
     */
    bool prepareToClose();
    bool saveAll();

    /* request close - used by individual pages */
    bool requestClose() override
    {
        if (m_container) {
            return m_container->requestClose();
        }
        return false;
    }

    bool selectPage(QString pageId) override;

    /** Returns the widget for the currently selected page - may be null if no page is selected */
    QWidget* selectedPageWidget() const override;

    /** Returns the info for the currently selected page - may be null if no page is selected */
    const PageInfo* selectedPageInfo() const;

    /** Returns the info for the page of the specified ID - may be null if the page has not yet been loaded */
    QWidget* pageWidget(QString pageId) const override;

    /** Returns a list where each index corresponds to the widget for the page at the same index provided by the BasePageProvider. Some
     * elements may be null. */
    const QList<QWidget*>& pageWidgets() const { return m_widgets; }

    // NOTE: Used to keep existing code working. This should not be used in new code.
    void forceLoadWidgets();

    void refreshContainer() override;
    virtual void setParentContainer(BasePageContainer* container) { m_container = container; };

    void changeEvent(QEvent*) override;

    void hidePageList() { m_pageList->hide(); }

   private:
    void createUI();
    void retranslate();
    void loadWidgetAt(int idx);

   public slots:
    void help() const;

   signals:
    /** Emitted when the currently selected page is changed */
    void selectedPageChanged(QWidget* prevWidget, QWidget* selectedWidget);

   private slots:
    void currentChanged(const QModelIndex& current);
    void showPage(int row);

   private:
    BasePageContainer* m_container = nullptr;
    int m_currentPage = -1;
    QSortFilterProxyModel* m_proxyModel;
    PageModel* m_model;
    QList<QWidget*> m_widgets;
    QStackedLayout* m_pageStack;
    QListView* m_pageList;
    QLabel* m_header;
    QGridLayout* m_layout;
};
