// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
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

#include "PageContainer.h"
#include "BuildConfig.h"
#include "PageContainer_p.h"

#include <QStackedLayout>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QUrl>
#include <QStyledItemDelegate>
#include <QListView>
#include <QLineEdit>
#include <QLabel>
#include <QDialogButtonBox>
#include <QGridLayout>

#include "settings/SettingsObject.h"

#include "ui/widgets/IconLabel.h"

#include "DesktopServices.h"
#include "Application.h"

class PageEntryFilterModel : public QSortFilterProxyModel
{
public:
    explicit PageEntryFilterModel(QObject *parent = 0) : QSortFilterProxyModel(parent)
    {
    }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
    {
        const QString pattern = filterRegularExpression().pattern();
        const auto model = static_cast<PageModel *>(sourceModel());
        const auto page = model->pages().at(sourceRow);
        if (!page->shouldDisplay())
            return false;
        // Regular contents check, then check page-filter.
        return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
    }
};

PageContainer::PageContainer(BasePageProvider *pageProvider, QString defaultId,
                             QWidget *parent)
    : QWidget(parent)
{
    createUI();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model = new PageModel(this);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_proxyModel = new PageEntryFilterModel(this);
    int counter = 0;
    auto pages = pageProvider->getPages();
    for (auto page : pages)
    {
        page->stackIndex = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pageStack->addWidget(dynamic_cast<QWidget *>(page));
        page->listIndex = counter;
        page->setParentContainer(this);
        counter++;
    }
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->setPages(pages);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_proxyModel->setSourceModel(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pageList->setIconSize(QSize(pageIconSize, pageIconSize));
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pageList->setSelectionMode(QAbstractItemView::SingleSelection);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pageList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pageList->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pageList->setModel(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_proxyModel);
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pageList->selectionModel(), SIGNAL(currentRowChanged(QModelIndex, QModelIndex)),
            this, SLOT(currentChanged(QModelIndex)));
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pageStack->setStackingMode(QStackedLayout::StackOne);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pageList->setFocus();
    selectPage(defaultId);
}

bool PageContainer::selectPage(QString pageId)
{
    // now find what we want to have selected...
    auto page = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->findPageEntryById(pageId);
    QModelIndex index;
    if (page)
    {
        index = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_proxyModel->mapFromSource(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->index(page->listIndex));
    }
    if(!index.isValid())
    {
        index = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_proxyModel->index(0, 0);
    }
    if (index.isValid())
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pageList->setCurrentIndex(index);
        return true;
    }
    return false;
}

BasePage* PageContainer::getPage(QString pageId)
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->findPageEntryById(pageId);
}

void PageContainer::refreshContainer()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_proxyModel->invalidate();
    if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentPage->shouldDisplay())
    {
        auto index = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_proxyModel->index(0, 0);
        if(index.isValid())
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pageList->setCurrentIndex(index);
        }
        else
        {
            // FIXME: unhandled corner case: what to do when there's no page to select?
        }
    }
}

void PageContainer::createUI()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pageStack = new QStackedLayout;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pageList = new PageView;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_header = new QLabel();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_iconHeader = new IconLabel(this, QIcon(), QSize(24, 24));

    QFont headerLabelFont = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_header->font();
    headerLabelFont.setBold(true);
    const int pointSize = headerLabelFont.pointSize();
    if (pointSize > 0)
        headerLabelFont.setPointSize(pointSize + 2);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_header->setFont(headerLabelFont);

    QHBoxLayout *headerHLayout = new QHBoxLayout;
    const int leftMargin = APPLICATION->style()->pixelMetric(QStyle::PM_LayoutLeftMargin);
    headerHLayout->addSpacerItem(new QSpacerItem(leftMargin, 0, QSizePolicy::Fixed, QSizePolicy::Ignored));
    headerHLayout->addWidget(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_header);
    headerHLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Ignored));
    headerHLayout->addWidget(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_iconHeader);
    const int rightMargin = APPLICATION->style()->pixelMetric(QStyle::PM_LayoutRightMargin);
    headerHLayout->addSpacerItem(new QSpacerItem(rightMargin, 0, QSizePolicy::Fixed, QSizePolicy::Ignored));
    headerHLayout->setContentsMargins(0, 6, 0, 0);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pageStack->setContentsMargins(0, 0, 0, 0);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pageStack->addWidget(new QWidget(this));

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_layout = new QGridLayout;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_layout->addLayout(headerHLayout, 0, 1, 1, 1);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_layout->addWidget(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pageList, 0, 0, 2, 1);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_layout->addLayout(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pageStack, 1, 1, 1, 1);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_layout->setColumnStretch(1, 4);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_layout->setContentsMargins(0,0,0,6);
    setLayout(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_layout);
}

void PageContainer::retranslate()
{
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentPage)
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_header->setText(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentPage->displayName());

    for (auto page : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->pages())
        page->retranslate();
}

void PageContainer::addButtons(QWidget *buttons)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_layout->addWidget(buttons, 2, 0, 1, 2);
}

void PageContainer::addButtons(QLayout *buttons)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_layout->addLayout(buttons, 2, 0, 1, 2);
}

void PageContainer::showPage(int row)
{
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentPage)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentPage->closed();
    }
    if (row != -1)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentPage = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->pages().at(row);
    }
    else
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentPage = nullptr;
    }
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentPage)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pageStack->setCurrentIndex(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentPage->stackIndex);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_header->setText(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentPage->displayName());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_iconHeader->setIcon(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentPage->icon());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentPage->opened();
    }
    else
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pageStack->setCurrentIndex(0);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_header->setText(QString());
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_iconHeader->setIcon(APPLICATION->getThemedIcon("bug"));
    }
}

void PageContainer::help()
{
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentPage)
    {
        QString pageId = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentPage->helpPage();
        if (pageId.isEmpty())
            return;
        DesktopServices::openUrl(QUrl(BuildConfig.HELP_URL.arg(pageId)));
    }
}

void PageContainer::currentChanged(const QModelIndex &current)
{
    int selected_index = current.isValid() ? hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_proxyModel->mapToSource(current).row() : -1;

    auto* selected = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->pages().at(selected_index);
    auto* previous = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentPage;

    emit selectedPageChanged(previous, selected);

    showPage(selected_index);
}

bool PageContainer::prepareToClose()
{
    if(!saveAll())
    {
        return false;
    }
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentPage)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentPage->closed();
    }
    return true;
}

bool PageContainer::saveAll()
{
    for (auto page : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->pages())
    {
        if (!page->apply())
            return false;
    }
    return true;
}

void PageContainer::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslate();
    }
    QWidget::changeEvent(event);
}
