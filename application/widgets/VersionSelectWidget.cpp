#include "VersionSelectWidget.h"
#include <QProgressBar>
#include <QVBoxLayout>
#include "VersionListView.h"
#include <QHeaderView>
#include <VersionProxyModel.h>
#include <dialogs/CustomMessageBox.h>

VersionSelectWidget::VersionSelectWidget(BaseVersionList* vlist, QWidget* parent)
	: QWidget(parent), m_vlist(vlist)
{
	setObjectName(QStringLiteral("VersionSelectWidget"));
	verticalLayout = new QVBoxLayout(this);
	verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
	verticalLayout->setContentsMargins(0, 0, 0, 0);

	m_proxyModel = new VersionProxyModel(this);
	m_proxyModel->setSourceModel(vlist);

	listView = new VersionListView(this);
	listView->setObjectName(QStringLiteral("listView"));
	listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	listView->setAlternatingRowColors(true);
	listView->setRootIsDecorated(false);
	listView->setItemsExpandable(false);
	listView->setWordWrap(true);
	listView->header()->setCascadingSectionResizes(true);
	listView->header()->setStretchLastSection(false);
	listView->setModel(m_proxyModel);
	listView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
	listView->header()->setSectionResizeMode(resizeOnColumn, QHeaderView::Stretch);
	verticalLayout->addWidget(listView);

	sneakyProgressBar = new QProgressBar(this);
	sneakyProgressBar->setObjectName(QStringLiteral("sneakyProgressBar"));
	sneakyProgressBar->setFormat(QStringLiteral("%p%"));
	verticalLayout->addWidget(sneakyProgressBar);
	sneakyProgressBar->setHidden(true);
	connect(listView->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &VersionSelectWidget::currentRowChanged);

	QMetaObject::connectSlotsByName(this);
}

void VersionSelectWidget::setEmptyString(QString emptyString)
{
	listView->setEmptyString(emptyString);
}

void VersionSelectWidget::setEmptyErrorString(QString emptyErrorString)
{
	listView->setEmptyErrorString(emptyErrorString);
}

VersionSelectWidget::~VersionSelectWidget()
{
}

void VersionSelectWidget::setResizeOn(int column)
{
	listView->header()->setSectionResizeMode(resizeOnColumn, QHeaderView::ResizeToContents);
	resizeOnColumn = column;
	listView->header()->setSectionResizeMode(resizeOnColumn, QHeaderView::Stretch);
}

void VersionSelectWidget::initialize()
{
	if (!m_vlist->isLoaded())
	{
		loadList();
	}
	else
	{
		if (m_proxyModel->rowCount() == 0)
		{
			listView->setEmptyMode(VersionListView::String);
		}
		preselect();
	}
}

void VersionSelectWidget::closeEvent(QCloseEvent * event)
{
	if(loadTask)
	{
		loadTask->abort();
		loadTask->deleteLater();
		loadTask = nullptr;
	}
	QWidget::closeEvent(event);
}

void VersionSelectWidget::loadList()
{
	if(loadTask)
	{
		return;
	}
	loadTask = m_vlist->getLoadTask();
	if (!loadTask)
	{
		return;
	}
	connect(loadTask, &Task::finished, this, &VersionSelectWidget::onTaskFinished);
	connect(loadTask, &Task::progress, this, &VersionSelectWidget::changeProgress);
	loadTask->start();
	sneakyProgressBar->setHidden(false);
}

void VersionSelectWidget::onTaskFinished()
{
	if (!loadTask->successful())
	{
		CustomMessageBox::selectable(this, tr("Error"),
									 tr("List update failed:\n%1").arg(loadTask->failReason()),
									 QMessageBox::Warning)->show();
		if (m_proxyModel->rowCount() == 0)
		{
			listView->setEmptyMode(VersionListView::ErrorString);
		}
	}
	else if (m_proxyModel->rowCount() == 0)
	{
		listView->setEmptyMode(VersionListView::String);
	}
	sneakyProgressBar->setHidden(true);
	loadTask->deleteLater();
	loadTask = nullptr;
	preselect();
}

void VersionSelectWidget::changeProgress(qint64 current, qint64 total)
{
	sneakyProgressBar->setMaximum(total);
	sneakyProgressBar->setValue(current);
}

void VersionSelectWidget::currentRowChanged(const QModelIndex& current, const QModelIndex&)
{
	auto variant = m_proxyModel->data(current, BaseVersionList::VersionPointerRole);
	emit selectedVersionChanged(variant.value<BaseVersionPtr>());
}

void VersionSelectWidget::preselect()
{
	if(preselectedAlready)
		return;
	preselectedAlready = true;
	selectRecommended();
}

void VersionSelectWidget::selectRecommended()
{
	auto idx = m_proxyModel->getRecommended();
	if(idx.isValid())
	{
		listView->selectionModel()->setCurrentIndex(idx,QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
		listView->scrollTo(idx, QAbstractItemView::PositionAtCenter);
	}
}

bool VersionSelectWidget::hasVersions() const
{
	return m_proxyModel->rowCount(QModelIndex()) != 0;
}

BaseVersionPtr VersionSelectWidget::selectedVersion() const
{
	auto currentIndex = listView->selectionModel()->currentIndex();
	auto variant = m_proxyModel->data(currentIndex, BaseVersionList::VersionPointerRole);
	return variant.value<BaseVersionPtr>();
}

void VersionSelectWidget::setExactFilter(BaseVersionList::ModelRoles role, QString filter)
{
	m_proxyModel->setFilter(role, filter, true);
}

void VersionSelectWidget::setFuzzyFilter(BaseVersionList::ModelRoles role, QString filter)
{
	m_proxyModel->setFilter(role, filter, false);
}