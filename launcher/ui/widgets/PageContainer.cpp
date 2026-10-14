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

#include <QAbstractButton>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPushButton>
#include <QRegularExpression>
#include <QShortcut>
#include <QSortFilterProxyModel>
#include <QStackedLayout>
#include <QStyledItemDelegate>
#include <QUrl>
#include <QVBoxLayout>
#include <utility>

#include "settings/SettingsObject.h"

#include "ui/widgets/IconLabel.h"

#include "Application.h"
#include "DesktopServices.h"
#include "ui/themes/ThemeManager.h"

class PageEntryFilterModel : public QSortFilterProxyModel {
   public:
    explicit PageEntryFilterModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent) {}

    void setSearch(const QString& text)
    {
        m_search = text.trimmed();
        invalidateRowsFilter();
    }
    // Page texts change with the language
    void forgetPageText() { m_pageText.clear(); }

   protected:
    bool filterAcceptsRow(int sourceRow, [[maybe_unused]] const QModelIndex& sourceParent) const override
    {
        auto* const model = static_cast<PageModel*>(sourceModel());
        auto* const page = model->pages().at(sourceRow);
        if (!page->shouldDisplay()) {
            return false;
        }
        if (m_search.isEmpty()) {
            return true;
        }
        return page->displayName().contains(m_search, Qt::CaseInsensitive) || pageText(page).contains(m_search, Qt::CaseInsensitive);
    }

   private:
    // Everything a person can read on a page: labels, buttons and group titles
    const QString& pageText(BasePage* page) const
    {
        auto it = m_pageText.find(page);
        if (it == m_pageText.end()) {
            QStringList parts;
            if (auto* widget = dynamic_cast<QWidget*>(page)) {
                for (auto* label : widget->findChildren<QLabel*>()) {
                    parts << label->text();
                }
                for (auto* button : widget->findChildren<QAbstractButton*>()) {
                    parts << button->text();
                }
                for (auto* group : widget->findChildren<QGroupBox*>()) {
                    parts << group->title();
                }
            }
            static const QRegularExpression s_markup(QStringLiteral("<[^>]*>"));
            it = m_pageText.insert(page, parts.join(' ').remove(s_markup).remove('&'));
        }
        return *it;
    }

    QString m_search;
    mutable QHash<BasePage*, QString> m_pageText;
};

PageContainer::PageContainer(BasePageProvider* pageProvider, QString defaultId, QWidget* parent)
    : QWidget(parent), m_proxyModel(new PageEntryFilterModel(this)), m_model(new PageModel(this))
{
    createUI();
    useSidebarStyle(true);

    int counter = 0;
    auto pages = pageProvider->getPages();
    for (auto* page : pages) {
        auto* widget = dynamic_cast<QWidget*>(page);
        widget->setParent(this);
        page->stackIndex = m_pageStack->addWidget(widget);
        page->listIndex = counter;
        page->setParentContainer(this);
        counter++;
        page->updateExtraInfo = [this](const QString& id, const QString& info) {
            if (m_currentPage && id == m_currentPage->id()) {
                m_header->setText(m_currentPage->displayName() + info);
            }
        };
    }
    m_model->setPages(pages);

    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    m_pageList->setIconSize(QSize(pageIconSize, pageIconSize));
    m_pageList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_pageList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_pageList->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    m_pageList->setModel(m_proxyModel);
    connect(m_pageList->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &PageContainer::currentChanged);
    m_pageStack->setStackingMode(QStackedLayout::StackOne);
    m_pageList->setFocus();
    selectPage(std::move(defaultId));
}

bool PageContainer::selectPage(QString pageId)
{
    // now find what we want to have selected...
    auto* page = m_model->findPageEntryById(pageId);
    QModelIndex index;
    if (page) {
        index = m_proxyModel->mapFromSource(m_model->index(page->listIndex));
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

BasePage* PageContainer::getPage(QString pageId)
{
    return m_model->findPageEntryById(pageId);
}

BasePage* PageContainer::selectedPage() const
{
    return m_currentPage;
}

const QList<BasePage*>& PageContainer::getPages() const
{
    return m_model->pages();
}

void PageContainer::refreshContainer()
{
    m_proxyModel->invalidate();
    if (!m_currentPage->shouldDisplay()) {
        auto index = m_proxyModel->index(0, 0);
        if (index.isValid()) {
            m_pageList->setCurrentIndex(index);
        } else {
            // FIXME: unhandled corner case: what to do when there's no page to select?
        }
    }
}

void PageContainer::createUI()
{
    m_pageStack = new QStackedLayout;
    m_pageList = new PageView;
    m_header = new QLabel();
    updateHeaderFont();

    // Finds pages by name or by any text on them, such as a setting's label
    m_search = new QLineEdit(this);
    m_search->setObjectName(QStringLiteral("pageSearch"));
    m_search->setClearButtonEnabled(true);
    connect(m_search, &QLineEdit::textChanged, this, &PageContainer::applySearch);
    auto* find = new QShortcut(QKeySequence::Find, this);
    connect(find, &QShortcut::activated, this, [this] {
        m_search->setFocus(Qt::ShortcutFocusReason);
        m_search->selectAll();
    });
    auto* clear = new QShortcut(QKeySequence(Qt::Key_Escape), m_search);
    clear->setContext(Qt::WidgetShortcut);
    connect(clear, &QShortcut::activated, m_search, &QLineEdit::clear);
    retranslateSearch();

    auto* headerHLayout = new QHBoxLayout;
    const int leftMargin = APPLICATION->style()->pixelMetric(QStyle::PM_LayoutLeftMargin);
    headerHLayout->addSpacerItem(new QSpacerItem(leftMargin, 0, QSizePolicy::Fixed, QSizePolicy::Ignored));
    headerHLayout->addWidget(m_header);
    headerHLayout->setContentsMargins(0, 6, 0, 0);

    m_pageStack->setContentsMargins(0, 0, 0, 0);
    m_pageStack->addWidget(new QWidget(this));

    auto* sidebar = new QVBoxLayout;
    sidebar->setSpacing(4);
    sidebar->setContentsMargins(4, 4, 0, 0);
    sidebar->addWidget(m_search);
    sidebar->addWidget(m_pageList);

    m_layout = new QGridLayout;
    m_layout->addLayout(headerHLayout, 0, 1, 1, 1);
    m_layout->addLayout(sidebar, 0, 0, 3, 1);
    m_layout->addLayout(m_pageStack, 1, 1, 1, 1);
    m_layout->setColumnStretch(1, 4);
    m_layout->setContentsMargins(0, 0, 0, 0);
    setLayout(m_layout);
}

void PageContainer::updateHeaderFont()
{
    QFont font = this->font();
    if (const auto& tokens = APPLICATION->themeManager()->getTokens(); !tokens.displayFont.isEmpty()) {
        font = QFont(tokens.displayFont);
        font.setPixelSize(qMax(18, tokens.displayFontPixelSize + 6));
    } else {
        font.setBold(true);
        const int pointSize = font.pointSize();
        if (pointSize > 0) {
            font.setPointSize(pointSize + 2);
        }
    }
    m_header->setFont(font);
}

void PageContainer::applySearch(const QString& text)
{
    static_cast<PageEntryFilterModel*>(m_proxyModel)->setSearch(text);
    // Keep showing a page that still matches; otherwise jump to the first one that does
    if (!m_pageList->currentIndex().isValid() && m_proxyModel->rowCount() > 0) {
        m_pageList->setCurrentIndex(m_proxyModel->index(0, 0));
    }
}

void PageContainer::retranslateSearch()
{
    m_search->setPlaceholderText(tr("Search"));
    m_search->setToolTip(
        tr("Find a page by its name or by any text on it (%1)").arg(QKeySequence(QKeySequence::Find).toString(QKeySequence::NativeText)));
}

void PageContainer::retranslate()
{
    retranslateSearch();
    static_cast<PageEntryFilterModel*>(m_proxyModel)->forgetPageText();
    if (m_currentPage) {
        m_header->setText(m_currentPage->displayName());
    }

    for (auto* page : m_model->pages()) {
        page->retranslate();
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
    // Search only makes sense for page lists, not for lists of choices such as instance sources
    m_search->setVisible(sidebar);
}

void PageContainer::showPage(int row)
{
    if (m_currentPage) {
        m_currentPage->closed();
    }
    if (row != -1) {
        m_currentPage = m_model->pages().at(row);
    } else {
        m_currentPage = nullptr;
    }
    if (m_currentPage) {
        m_pageStack->setCurrentIndex(m_currentPage->stackIndex);
        m_header->setText(m_currentPage->displayName());
        m_currentPage->opened();
    } else {
        m_pageStack->setCurrentIndex(0);
        m_header->setText(QString());
    }
}

void PageContainer::help()
{
    if (m_currentPage) {
        QString pageId = m_currentPage->helpPage();
        if (pageId.isEmpty()) {
            return;
        }
        DesktopServices::openUrl(QUrl(BuildConfig.HELP_URL.arg(pageId)));
    }
}

void PageContainer::currentChanged(const QModelIndex& current)
{
    if (!current.isValid()) {
        // The current page was filtered out by a search; keep showing it until another is picked
        return;
    }
    int selectedIndex = m_proxyModel->mapToSource(current).row();

    Q_ASSERT(selectedIndex < m_model->pages().size() && selectedIndex >= 0);

    auto* selected = m_model->pages().at(selectedIndex);
    auto* previous = m_currentPage;

    emit selectedPageChanged(previous, selected);

    showPage(selectedIndex);
}

bool PageContainer::prepareToClose()
{
    if (!saveAll()) {
        return false;
    }
    if (m_currentPage) {
        m_currentPage->closed();
    }
    return true;
}

bool PageContainer::saveAll()
{
    for (auto* page : m_model->pages()) {
        if (!page->apply()) {
            return false;
        }
    }
    return true;
}

void PageContainer::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslate();
    } else if (event->type() == QEvent::StyleChange) {
        updateHeaderFont();
    }
    QWidget::changeEvent(event);
}
