/* Copyright 2014 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "PageDialog.h"
#include "gui/Platform.h"
#include <QStackedLayout>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QUrl>
#include "MultiMC.h"
#include <QStyledItemDelegate>
#include <QListView>
#include <QLineEdit>
#include <QLabel>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QDesktopServices>
#include <settingsobject.h>

#include "PageDialog_p.h"
#include <gui/widgets/IconLabel.h>

class PageEntryFilterModel : public QSortFilterProxyModel
{
public:
	explicit PageEntryFilterModel(QObject *parent = 0) : QSortFilterProxyModel(parent)
	{
	}

protected:
	bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
	{
		const QString pattern = filterRegExp().pattern();
		const auto model = static_cast<PageModel *>(sourceModel());
		const auto page = model->pages().at(sourceRow);
		if(!page->shouldDisplay())
			return false;
		// Regular contents check, then check page-filter.
		return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
	}
};

PageDialog::PageDialog(BasePageProviderPtr pageProvider, QString defaultId, QWidget *parent) : QDialog(parent)
{
	MultiMCPlatform::fixWM_CLASS(this);
	createUI();
	setWindowTitle(pageProvider->dialogTitle());
	restoreGeometry(QByteArray::fromBase64(MMC->settings()->get("PagedGeometry").toByteArray()));

	m_model = new PageModel(this);
	m_proxyModel = new PageEntryFilterModel(this);
	int firstIndex = -1;
	int counter = 0;
	auto pages = pageProvider->getPages();
	for(auto page: pages)
	{
		page->stackIndex = m_pageStack->addWidget(dynamic_cast<QWidget *>(page));
		page->listIndex = counter;
		counter++;
		if(firstIndex == -1)
		{
			firstIndex = page->stackIndex;
		}
	}
	m_model->setPages(pages);

	m_proxyModel->setSourceModel(m_model);
	m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

	m_pageList->setIconSize(QSize(pageIconSize, pageIconSize));
	m_pageList->setSelectionMode(QAbstractItemView::SingleSelection);
	m_pageList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_pageList->setModel(m_proxyModel);
    connect(m_pageList->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this, SLOT(currentChanged(QModelIndex)));
	m_pageStack->setStackingMode(QStackedLayout::StackOne);
	m_pageList->setFocus();
	// now find what we want to have selected...
	auto page = m_model->findPageEntryById(defaultId);
	QModelIndex index;
	if(page)
	{
		index = m_proxyModel->mapFromSource(m_model->index(page->listIndex));
	}
	else
	{
		index = m_proxyModel->index(0,0);
	}
	if(index.isValid())
		m_pageList->setCurrentIndex(index);
}

void PageDialog::createUI()
{
	m_pageStack = new QStackedLayout;
	m_filter = new QLineEdit;
	m_pageList = new PageView;
	m_header = new QLabel();
	m_iconHeader = new IconLabel(this, QIcon(), QSize(24,24));

	QFont headerLabelFont = m_header->font();
	headerLabelFont.setBold(true);
	const int pointSize = headerLabelFont.pointSize();
	if (pointSize > 0)
		headerLabelFont.setPointSize(pointSize + 2);
	m_header->setFont(headerLabelFont);

	QHBoxLayout *headerHLayout = new QHBoxLayout;
	const int leftMargin = MMC->style()->pixelMetric(QStyle::PM_LayoutLeftMargin);
	headerHLayout->addSpacerItem(
		new QSpacerItem(leftMargin, 0, QSizePolicy::Fixed, QSizePolicy::Ignored));
	headerHLayout->addWidget(m_header);
	headerHLayout->addSpacerItem(
		new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Ignored));
	headerHLayout->addWidget(m_iconHeader);

	m_pageStack->setMargin(0);
	m_pageStack->addWidget(new QWidget(this));

	QDialogButtonBox *buttons =
		new QDialogButtonBox(QDialogButtonBox::Help | QDialogButtonBox::Close);
	buttons->button(QDialogButtonBox::Close)->setDefault(true);
	connect(buttons->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(close()));
	connect(buttons->button(QDialogButtonBox::Help), SIGNAL(clicked()), this, SLOT(help()));

	QGridLayout *mainGridLayout = new QGridLayout;
	mainGridLayout->addLayout(headerHLayout, 0, 1, 1, 1);
	mainGridLayout->addWidget(m_pageList, 0, 0, 2, 1);
	mainGridLayout->addLayout(m_pageStack, 1, 1, 1, 1);
	mainGridLayout->addWidget(buttons, 2, 0, 1, 2);
	mainGridLayout->setColumnStretch(1, 4);
	setLayout(mainGridLayout);
}

void PageDialog::showPage(int row)
{
	if(row != -1)
	{
		m_currentPage = m_model->pages().at(row);
	}
	else
	{
		m_currentPage = nullptr;
	}
	if(m_currentPage)
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
		m_iconHeader->setIcon(QIcon::fromTheme("bug"));
	}
}

void PageDialog::help()
{
	if(m_currentPage)
	{
		QString pageId = m_currentPage->helpPage();
		if(pageId.isEmpty())
			return;
		QDesktopServices::openUrl(QUrl("https://github.com/MultiMC/MultiMC5/wiki/" + pageId));
	}
}

void PageDialog::currentChanged(const QModelIndex &current)
{
	showPage(current.isValid() ? m_proxyModel->mapToSource(current).row() : -1);
}

void PageDialog::closeEvent(QCloseEvent * event)
{
	bool accepted = true;
	for(auto page: m_model->pages())
	{
		accepted &= page->apply();
	}
	if(accepted)
	{
		MMC->settings()->set("PagedGeometry", saveGeometry().toBase64());
		QDialog::closeEvent(event);
	}
}
