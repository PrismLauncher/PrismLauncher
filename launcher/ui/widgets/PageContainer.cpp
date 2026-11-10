// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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

#include "PageContainer.h"
#include "BuildConfig.h"
#include "PageContainer_p.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStackedLayout>
#include <QStyledItemDelegate>
#include <QUrl>
#include <utility>

#include "settings/SettingsObject.h"

#include "ui/widgets/IconLabel.h"

#include "Application.h"
#include "DesktopServices.h"

class PageEntryFilterModel : public QSortFilterProxyModel {
   public:
    explicit PageEntryFilterModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent) {}

   protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override
    {
        const QString pattern = filterRegularExpression().pattern();
        auto* const model = static_cast<PageModel*>(sourceModel());
        const auto& page = model->pages().at(sourceRow);
        if (page.shouldDisplay != nullptr && !page.shouldDisplay()) {
            return false;
        }
        // Regular contents check, then check page-filter.
        return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
    }
};

PageContainer::PageContainer(BasePageProvider* pageProvider, QString defaultId, QWidget* parent)
    : QWidget(parent), m_proxyModel(new PageEntryFilterModel(this)), m_model(new PageModel(this))
{
    createUI();
    useSidebarStyle(true);

    m_model->setPages(pageProvider->getPages());
    m_widgets.resize(m_model->pages().size());
    for (int i = 0; i < m_model->pages().size(); ++i) {
        // NOTE: reserve the space for when the pages are lazily loaded
        m_pageStack->addWidget(new QWidget(this));
    }

    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    m_pageList->setIconSize(QSize(pageIconSize, pageIconSize));
    m_pageList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_pageList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_pageList->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    m_pageList->setModel(m_proxyModel);
    connect(m_pageList->selectionModel(), &QItemSelectionModel::currentChanged, this, &PageContainer::currentChanged);
    m_pageList->setFocus();
    selectPage(std::move(defaultId));
}

bool PageContainer::selectPage(QString pageId)
{
    // now find what we want to have selected...
    const auto pageIndex = m_model->findPageIndex(pageId);
    QModelIndex index;
    if (pageIndex != -1) {
        index = m_proxyModel->mapFromSource(m_model->index(pageIndex));
    }
    if (!index.isValid()) {
        index = m_proxyModel->index(0, 0);
    }
    if (index.isValid()) {
        m_pageList->setCurrentIndex(index);
        return true;
    }
    return false;
}

QWidget* PageContainer::selectedPageWidget() const
{
    if (m_currentPage < 0) {
        return nullptr;
    }

    return m_widgets.at(m_currentPage);
}

const PageInfo* PageContainer::selectedPageInfo() const
{
    if (m_currentPage < 0) {
        return nullptr;
    }

    return &m_model->pages().at(m_currentPage);
}

QWidget* PageContainer::pageWidget(QString pageId) const
{
    const auto pageIndex = m_model->findPageIndex(pageId);
    if (pageIndex == -1) {
        return nullptr;
    }

    return m_widgets.at(pageIndex);
}

void PageContainer::forceLoadWidgets()
{
    for (int i = 0; i < m_widgets.size(); ++i) {
        loadWidgetAt(i);
    }
}

void PageContainer::refreshContainer()
{
    m_proxyModel->invalidate();
    const auto* currentPage = selectedPageInfo();
    if (currentPage == nullptr || currentPage->shouldDisplay == nullptr || currentPage->shouldDisplay()) {
        return;
    }

    auto index = m_proxyModel->index(0, 0);
    if (index.isValid()) {
        m_pageList->setCurrentIndex(index);
    } else {
        m_pageList->clearSelection();
    }
}

void PageContainer::createUI()
{
    m_pageStack = new QStackedLayout;
    m_pageList = new PageView;
    m_header = new QLabel();

    QFont headerLabelFont = m_header->font();
    headerLabelFont.setBold(true);
    const int pointSize = headerLabelFont.pointSize();
    if (pointSize > 0) {
        headerLabelFont.setPointSize(pointSize + 2);
    }
    m_header->setFont(headerLabelFont);

    auto* headerHLayout = new QHBoxLayout;
    const int leftMargin = APPLICATION->style()->pixelMetric(QStyle::PM_LayoutLeftMargin);
    headerHLayout->addSpacerItem(new QSpacerItem(leftMargin, 0, QSizePolicy::Fixed, QSizePolicy::Ignored));
    headerHLayout->addWidget(m_header);
    headerHLayout->setContentsMargins(0, 6, 0, 0);

    m_pageStack->setContentsMargins(0, 0, 0, 0);
    m_pageStack->addWidget(new QWidget(this));

    m_layout = new QGridLayout;
    m_layout->addLayout(headerHLayout, 0, 1, 1, 1);
    m_layout->addWidget(m_pageList, 0, 0, 3, 1);
    m_layout->addLayout(m_pageStack, 1, 1, 1, 1);
    m_layout->setColumnStretch(1, 4);
    m_layout->setContentsMargins(0, 0, 0, 0);
    setLayout(m_layout);
}

void PageContainer::retranslate()
{
    if (const auto* info = selectedPageInfo(); info != nullptr) {
        m_header->setText(QObject::tr(info->displayName));
    }

    for (auto* page : m_widgets) {
        auto* basePage = dynamic_cast<BasePage*>(page);
        if (basePage == nullptr) {
            continue;
        }

        basePage->retranslate();
    }
}

void PageContainer::addButtons(QWidget* buttons)
{
    m_layout->addWidget(buttons, 2, 1, 1, 2);
}

void PageContainer::addButtons(QLayout* buttons)
{
    m_layout->addLayout(buttons, 2, 1, 1, 2);
}

void PageContainer::useSidebarStyle(bool sidebar)
{
    m_pageList->setProperty("_kde_side_panel_view", sidebar);
}

void PageContainer::showPage(int row)
{
    if (auto* basePage = dynamic_cast<BasePage*>(selectedPageWidget()); basePage != nullptr) {
        basePage->closed();
    }
    m_currentPage = row;

    // NOTE: passing -1 is safe
    m_pageStack->setCurrentIndex(m_currentPage);

    if (const auto* info = selectedPageInfo(); info != nullptr) {
        m_header->setText(QObject::tr(info->displayName));

        loadWidgetAt(m_currentPage);
    }
    if (auto* basePage = dynamic_cast<BasePage*>(selectedPageWidget()); basePage != nullptr) {
        basePage->opened();
    }
}

void PageContainer::loadWidgetAt(int idx)
{
    if (m_widgets.at(idx) != nullptr) {
        return;
    }

    QWidget* containerWidget = m_pageStack->widget(idx);
    Q_ASSERT(containerWidget != nullptr);

    const auto info = m_model->pages().at(idx);
    QWidget* widget = info.constructor(containerWidget);
    m_widgets[idx] = widget;
    // NOTE: needed as some pages are QMainWindow (?) and without this they appear as separate windows instead of inside the page depsite
    // the same parent being passed through the constructor
    // FIXME: move away from using QMainWindow for pages
    widget->setParent(this);

    auto* layout = new QVBoxLayout(containerWidget);
    layout->setContentsMargins(0, 0, 0, 0);

    layout->addWidget(widget);

    if (auto* basePage = dynamic_cast<BasePage*>(widget); basePage != nullptr) {
        // FIXME: hacky, breaks with retranslate
        // pages should probably display extra info inside their own area instead
        basePage->updateExtraInfo = [this, widget](const QString& info) {
            if (selectedPageWidget() == widget) {
                m_header->setText(tr(selectedPageInfo()->displayName) + info);
            }
        };
        basePage->setParentContainer(this);
    }
}

void PageContainer::help() const
{
    const auto* info = selectedPageInfo();
    if (info == nullptr) {
        return;
    }

    DesktopServices::openUrl(QUrl(BuildConfig.HELP_URL.arg(info->id)));
}

void PageContainer::currentChanged(const QModelIndex& current)
{
    int selectedIndex = current.isValid() ? m_proxyModel->mapToSource(current).row() : -1;

    QWidget* currentWidget = selectedIndex >= 0 ? m_widgets.at(selectedIndex) : nullptr;
    auto* prevWidget = selectedPageWidget();

    emit selectedPageChanged(prevWidget, currentWidget);

    showPage(selectedIndex);
}

bool PageContainer::prepareToClose()
{
    if (!saveAll()) {
        return false;
    }

    if (auto* basePage = dynamic_cast<BasePage*>(selectedPageWidget()); basePage != nullptr) {
        basePage->closed();
    }
    return true;
}

bool PageContainer::saveAll()
{
    for (auto* page : m_widgets) {
        auto* basePage = dynamic_cast<BasePage*>(page);
        if (basePage == nullptr) {
            continue;
        }

        if (!basePage->apply()) {
            return false;
        }
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
