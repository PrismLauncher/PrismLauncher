/* Copyright 2015 MultiMC Contributors
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

#include "WonkoPage.h"
#include "ui_WonkoPage.h"

#include <QDateTime>
#include <QSortFilterProxyModel>
#include <QRegularExpression>

#include "dialogs/ProgressDialog.h"
#include "VersionProxyModel.h"

#include "wonko/WonkoIndex.h"
#include "wonko/WonkoVersionList.h"
#include "wonko/WonkoVersion.h"
#include "Env.h"
#include "MultiMC.h"

static QString formatRequires(const WonkoVersionPtr &version)
{
	QStringList lines;
	for (const WonkoReference &ref : version->requires())
	{
		const QString readable = ENV.wonkoIndex()->hasUid(ref.uid()) ? ENV.wonkoIndex()->getList(ref.uid())->humanReadable() : ref.uid();
		if (ref.version().isEmpty())
		{
			lines.append(readable);
		}
		else
		{
			lines.append(QString("%1 (%2)").arg(readable, ref.version()));
		}
	}
	return lines.join('\n');
}

WonkoPage::WonkoPage(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::WonkoPage)
{
	ui->setupUi(this);
	ui->tabWidget->tabBar()->hide();

	m_fileProxy = new QSortFilterProxyModel(this);
	m_fileProxy->setSortRole(Qt::DisplayRole);
	m_fileProxy->setSortCaseSensitivity(Qt::CaseInsensitive);
	m_fileProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
	m_fileProxy->setFilterRole(Qt::DisplayRole);
	m_fileProxy->setFilterKeyColumn(0);
	m_fileProxy->sort(0);
	m_fileProxy->setSourceModel(ENV.wonkoIndex().get());
	ui->indexView->setModel(m_fileProxy);

	m_filterProxy = new QSortFilterProxyModel(this);
	m_filterProxy->setSortRole(WonkoVersionList::SortRole);
	m_filterProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
	m_filterProxy->setFilterRole(Qt::DisplayRole);
	m_filterProxy->setFilterKeyColumn(0);
	m_filterProxy->sort(0, Qt::DescendingOrder);
	ui->versionsView->setModel(m_filterProxy);

	m_versionProxy = new VersionProxyModel(this);
	m_filterProxy->setSourceModel(m_versionProxy);

	connect(ui->indexView->selectionModel(), &QItemSelectionModel::currentChanged, this, &WonkoPage::updateCurrentVersionList);
	connect(ui->versionsView->selectionModel(), &QItemSelectionModel::currentChanged, this, &WonkoPage::updateVersion);
	connect(m_filterProxy, &QSortFilterProxyModel::dataChanged, this, &WonkoPage::versionListDataChanged);

	updateCurrentVersionList(QModelIndex());
	updateVersion();
}

WonkoPage::~WonkoPage()
{
	delete ui;
}

QIcon WonkoPage::icon() const
{
	return MMC->getThemedIcon("looney");
}

void WonkoPage::on_refreshIndexBtn_clicked()
{
	ProgressDialog(this).execWithTask(ENV.wonkoIndex()->remoteUpdateTask());
}
void WonkoPage::on_refreshFileBtn_clicked()
{
	WonkoVersionListPtr list = ui->indexView->currentIndex().data(WonkoIndex::ListPtrRole).value<WonkoVersionListPtr>();
	if (!list)
	{
		return;
	}
	ProgressDialog(this).execWithTask(list->remoteUpdateTask());
}
void WonkoPage::on_refreshVersionBtn_clicked()
{
	WonkoVersionPtr version = ui->versionsView->currentIndex().data(WonkoVersionList::WonkoVersionPtrRole).value<WonkoVersionPtr>();
	if (!version)
	{
		return;
	}
	ProgressDialog(this).execWithTask(version->remoteUpdateTask());
}

void WonkoPage::on_fileSearchEdit_textChanged(const QString &search)
{
	if (search.isEmpty())
	{
		m_fileProxy->setFilterFixedString(QString());
	}
	else
	{
		QStringList parts = search.split(' ');
		std::transform(parts.begin(), parts.end(), parts.begin(), &QRegularExpression::escape);
		m_fileProxy->setFilterRegExp(".*" + parts.join(".*") + ".*");
	}
}
void WonkoPage::on_versionSearchEdit_textChanged(const QString &search)
{
	if (search.isEmpty())
	{
		m_filterProxy->setFilterFixedString(QString());
	}
	else
	{
		QStringList parts = search.split(' ');
		std::transform(parts.begin(), parts.end(), parts.begin(), &QRegularExpression::escape);
		m_filterProxy->setFilterRegExp(".*" + parts.join(".*") + ".*");
	}
}

void WonkoPage::updateCurrentVersionList(const QModelIndex &index)
{
	if (index.isValid())
	{
		WonkoVersionListPtr list = index.data(WonkoIndex::ListPtrRole).value<WonkoVersionListPtr>();
		ui->versionsBox->setEnabled(true);
		ui->refreshFileBtn->setEnabled(true);
		ui->fileUidLabel->setEnabled(true);
		ui->fileUid->setText(list->uid());
		ui->fileNameLabel->setEnabled(true);
		ui->fileName->setText(list->name());
		m_versionProxy->setSourceModel(list.get());
		ui->refreshFileBtn->setText(tr("Refresh %1").arg(list->humanReadable()));

		if (!list->isLocalLoaded())
		{
			std::unique_ptr<Task> task = list->localUpdateTask();
			connect(task.get(), &Task::finished, this, [this, list]()
			{
				if (list->count() == 0 && !list->isRemoteLoaded())
				{
					ProgressDialog(this).execWithTask(list->remoteUpdateTask());
				}
			});
			ProgressDialog(this).execWithTask(task);
		}
	}
	else
	{
		ui->versionsBox->setEnabled(false);
		ui->refreshFileBtn->setEnabled(false);
		ui->fileUidLabel->setEnabled(false);
		ui->fileUid->clear();
		ui->fileNameLabel->setEnabled(false);
		ui->fileName->clear();
		m_versionProxy->setSourceModel(nullptr);
		ui->refreshFileBtn->setText(tr("Refresh ___"));
	}
}

void WonkoPage::versionListDataChanged(const QModelIndex &tl, const QModelIndex &br)
{
	if (QItemSelection(tl, br).contains(ui->versionsView->currentIndex()))
	{
		updateVersion();
	}
}

void WonkoPage::updateVersion()
{
	WonkoVersionPtr version = std::dynamic_pointer_cast<WonkoVersion>(
				ui->versionsView->currentIndex().data(WonkoVersionList::VersionPointerRole).value<BaseVersionPtr>());
	if (version)
	{
		ui->refreshVersionBtn->setEnabled(true);
		ui->versionVersionLabel->setEnabled(true);
		ui->versionVersion->setText(version->version());
		ui->versionTimeLabel->setEnabled(true);
		ui->versionTime->setText(version->time().toString("yyyy-MM-dd HH:mm"));
		ui->versionTypeLabel->setEnabled(true);
		ui->versionType->setText(version->type());
		ui->versionRequiresLabel->setEnabled(true);
		ui->versionRequires->setText(formatRequires(version));
		ui->refreshVersionBtn->setText(tr("Refresh %1").arg(version->version()));
	}
	else
	{
		ui->refreshVersionBtn->setEnabled(false);
		ui->versionVersionLabel->setEnabled(false);
		ui->versionVersion->clear();
		ui->versionTimeLabel->setEnabled(false);
		ui->versionTime->clear();
		ui->versionTypeLabel->setEnabled(false);
		ui->versionType->clear();
		ui->versionRequiresLabel->setEnabled(false);
		ui->versionRequires->clear();
		ui->refreshVersionBtn->setText(tr("Refresh ___"));
	}
}

void WonkoPage::opened()
{
	if (!ENV.wonkoIndex()->isLocalLoaded())
	{
		std::unique_ptr<Task> task = ENV.wonkoIndex()->localUpdateTask();
		connect(task.get(), &Task::finished, this, [this]()
		{
			if (!ENV.wonkoIndex()->isRemoteLoaded())
			{
				ProgressDialog(this).execWithTask(ENV.wonkoIndex()->remoteUpdateTask());
			}
		});
		ProgressDialog(this).execWithTask(task);
	}
}
