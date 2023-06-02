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
    m_model = new PageModel(this);
    m_proxyModel = new PageEntryFilterModel(this);
    int counter = 0;
    auto pages = pageProvider->getPages();
    for (auto page : pages)
    {
        auto widget = dynamic_cast<QWidget *>(page);
        widget->setParent(this);
        page->stackIndex = m_pageStack->addWidget(widget);
        page->listIndex = counter;
        page->setParentContainer(this);
        counter++;
    }
    m_model->setPages(pages);

    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    m_pageList->setIconSize(QSize(pageIconSize, pageIconSize));
    m_pageList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_pageList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_pageList->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    m_pageList->setModel(m_proxyModel);
    connect(m_pageList->selectionModel(), SIGNAL(currentRowChanged(QModelIndex, QModelIndex)),
            this, SLOT(currentChanged(QModelIndex)));
    m_pageStack->setStackingMode(QStackedLayout::StackOne);
    m_pageList->setFocus();
    selectPage(defaultId);
}

bool PageContainer::selectPage(QString pageId)
{
    // now find what we want to have selected...
    auto page = m_model->findPageEntryById(pageId);
    QModelIndex index;
    if (page)
    {
        index = m_proxyModel->mapFromSource(m_model->index(page->listIndex));
    }
    if(!index.isValid())
    {
        index = m_proxyModel->index(0, 0);
    }
    if (index.isValid())
    {
        m_pageList->setCurrentIndex(index);
        return true;
    }
    return false;
}

BasePage* PageContainer::getPage(QString pageId)
{
    return m_model->findPageEntryById(pageId);
}

const QList<BasePage*> PageContainer::getPages() const
{
    return m_model->pages();
}

void PageContainer::refreshContainer()
{
    m_proxyModel->invalidate();
    if(!m_currentPage->shouldDisplay())
    {
        auto index = m_proxyModel->index(0, 0);
        if(index.isValid())
        {
            m_pageList->setCurrentIndex(index);
        }
        else
        {
            // FIXME: unhandled corner case: what to do when there's no page to select?
        }
    }
}

void PageContainer::createUI()
{
    m_pageStack = new QStackedLayout;
    m_pageList = new PageView;
    m_header = new QLabel();
    m_iconHeader = new IconLabel(this, QIcon(), QSize(24, 24));

    QFont headerLabelFont = m_header->font();
    headerLabelFont.setBold(true);
    const int pointSize = headerLabelFont.pointSize();
    if (pointSize > 0)
        headerLabelFont.setPointSize(pointSize + 2);
    m_header->setFont(headerLabelFont);

    QHBoxLayout *headerHLayout = new QHBoxLayout;
    const int leftMargin = APPLICATION->style()->pixelMetric(QStyle::PM_LayoutLeftMargin);
    headerHLayout->addSpacerItem(new QSpacerItem(leftMargin, 0, QSizePolicy::Fixed, QSizePolicy::Ignored));
    headerHLayout->addWidget(m_header);
    headerHLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Ignored));
    headerHLayout->addWidget(m_iconHeader);
    const int rightMargin = APPLICATION->style()->pixelMetric(QStyle::PM_LayoutRightMargin);
    headerHLayout->addSpacerItem(new QSpacerItem(rightMargin, 0, QSizePolicy::Fixed, QSizePolicy::Ignored));
    headerHLayout->setContentsMargins(0, 6, 0, 0);

    m_pageStack->setContentsMargins(0, 0, 0, 0);
    m_pageStack->addWidget(new QWidget(this));

    m_layout = new QGridLayout;
    m_layout->addLayout(headerHLayout, 0, 1, 1, 1);
    m_layout->addWidget(m_pageList, 0, 0, 2, 1);
    m_layout->addLayout(m_pageStack, 1, 1, 1, 1);
    m_layout->setColumnStretch(1, 4);
    m_layout->setContentsMargins(0,0,0,6);
    setLayout(m_layout);
}

void PageContainer::retranslate()
{
    if (m_currentPage)
        m_header->setText(m_currentPage->displayName());

    for (auto page : m_model->pages())
        page->retranslate();
}

void PageContainer::addButtons(QWidget *buttons)
{
    m_layout->addWidget(buttons, 2, 0, 1, 2);
}

void PageContainer::addButtons(QLayout *buttons)
{
    m_layout->addLayout(buttons, 2, 0, 1, 2);
}

void PageContainer::showPage(int row)
{
    if (m_currentPage)
    {
        m_currentPage->closed();
    }
    if (row != -1)
    {
        m_currentPage = m_model->pages().at(row);
    }
    else
    {
        m_currentPage = nullptr;
    }
    if (m_currentPage)
    {
        m_pageStack->setCurrentIndex(m_currentPage->stackIndex);
        m_header->setText(m_currentPage->displayName());
        m_iconHeader->setIcon(m_currentPage->icon());
        m_currentPage->opened();
    }
    else
    {
        m_pageStack->setCurrentIndex(0);
        m_header->setText(QString());
        m_iconHeader->setIcon(APPLICATION->getThemedIcon("bug"));
    }
}

void PageContainer::help()
{
    if (m_currentPage)
    {
        QString pageId = m_currentPage->helpPage();
        if (pageId.isEmpty())
            return;
        DesktopServices::openUrl(QUrl(BuildConfig.HELP_URL.arg(pageId)));
    }
}

void PageContainer::currentChanged(const QModelIndex &current)
{
    int selected_index = current.isValid() ? m_proxyModel->mapToSource(current).row() : -1;

    auto* selected = m_model->pages().at(selected_index);
    auto* previous = m_currentPage;

    emit selectedPageChanged(previous, selected);

    showPage(selected_index);
}

bool PageContainer::prepareToClose()
{
    if(!saveAll())
    {
        return false;
    }
    if (m_currentPage)
    {
        m_currentPage->closed();
    }
    return true;
}

bool PageContainer::saveAll()
{
    for (auto page : m_model->pages())
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
