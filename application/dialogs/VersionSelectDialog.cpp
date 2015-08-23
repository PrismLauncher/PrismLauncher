/* Copyright 2013-2015 MultiMC Contributors
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

#include "VersionSelectDialog.h"
#include "ui_VersionSelectDialog.h"

#include <QHeaderView>

#include <dialogs/ProgressDialog.h>
#include "CustomMessageBox.h"

#include <BaseVersion.h>
#include <BaseVersionList.h>
#include <tasks/Task.h>
#include <modutils.h>
#include <QDebug>
#include "MultiMC.h"
#include <VersionProxyModel.h>

VersionSelectDialog::VersionSelectDialog(BaseVersionList *vlist, QString title, QWidget *parent,
										 bool cancelable)
	: QDialog(parent), ui(new Ui::VersionSelectDialog)
{
	ui->setupUi(this);
	setWindowModality(Qt::WindowModal);
	setWindowTitle(title);

	m_vlist = vlist;

	m_proxyModel = new VersionProxyModel(this);
	m_proxyModel->setSourceModel(vlist);

	ui->listView->setModel(m_proxyModel);
	ui->listView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
	ui->listView->header()->setSectionResizeMode(resizeOnColumn, QHeaderView::Stretch);
	ui->sneakyProgressBar->setHidden(true);

	if (!cancelable)
	{
		ui->buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(false);
	}
}

void VersionSelectDialog::setEmptyString(QString emptyString)
{
	ui->listView->setEmptyString(emptyString);
}

void VersionSelectDialog::setEmptyErrorString(QString emptyErrorString)
{
	ui->listView->setEmptyErrorString(emptyErrorString);
}

VersionSelectDialog::~VersionSelectDialog()
{
	delete ui;
}

void VersionSelectDialog::setResizeOn(int column)
{
	ui->listView->header()->setSectionResizeMode(resizeOnColumn, QHeaderView::ResizeToContents);
	resizeOnColumn = column;
	ui->listView->header()->setSectionResizeMode(resizeOnColumn, QHeaderView::Stretch);
}

int VersionSelectDialog::exec()
{
	QDialog::open();
	if (!m_vlist->isLoaded())
	{
		loadList();
	}
	else
	{
		if (m_proxyModel->rowCount() == 0)
		{
			ui->listView->setEmptyMode(VersionListView::String);
		}
		preselect();
	}
	return QDialog::exec();
}

void VersionSelectDialog::closeEvent(QCloseEvent * event)
{
	if(loadTask)
	{
		loadTask->abort();
		loadTask->deleteLater();
		loadTask = nullptr;
	}
	QDialog::closeEvent(event);
}

void VersionSelectDialog::loadList()
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
	connect(loadTask, &Task::finished, this, &VersionSelectDialog::onTaskFinished);
	connect(loadTask, &Task::progress, this, &VersionSelectDialog::changeProgress);
	loadTask->start();
	ui->sneakyProgressBar->setHidden(false);
}

void VersionSelectDialog::onTaskFinished()
{
	if (!loadTask->successful())
	{
		CustomMessageBox::selectable(this, tr("Error"),
									 tr("List update failed:\n%1").arg(loadTask->failReason()),
									 QMessageBox::Warning)->show();
		if (m_proxyModel->rowCount() == 0)
		{
			ui->listView->setEmptyMode(VersionListView::ErrorString);
		}
	}
	else if (m_proxyModel->rowCount() == 0)
	{
		ui->listView->setEmptyMode(VersionListView::String);
	}
	ui->sneakyProgressBar->setHidden(true);
	loadTask->deleteLater();
	loadTask = nullptr;
	preselect();
}

void VersionSelectDialog::changeProgress(qint64 current, qint64 total)
{
	ui->sneakyProgressBar->setMaximum(total);
	ui->sneakyProgressBar->setValue(current);
}

void VersionSelectDialog::preselect()
{
	if(preselectedAlready)
		return;
	preselectedAlready = true;
	selectRecommended();
}

void VersionSelectDialog::selectRecommended()
{
	auto idx = m_proxyModel->getRecommended();
	if(idx.isValid())
	{
		ui->listView->selectionModel()->setCurrentIndex(idx,QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
		ui->listView->scrollTo(idx, QAbstractItemView::PositionAtCenter);
	}
}

BaseVersionPtr VersionSelectDialog::selectedVersion() const
{
	auto currentIndex = ui->listView->selectionModel()->currentIndex();
	auto variant = m_proxyModel->data(currentIndex, BaseVersionList::VersionPointerRole);
	return variant.value<BaseVersionPtr>();
}

void VersionSelectDialog::on_refreshButton_clicked()
{
	loadList();
}

void VersionSelectDialog::setExactFilter(BaseVersionList::ModelRoles role, QString filter)
{
	m_proxyModel->setFilter(role, filter, true);
}

void VersionSelectDialog::setFuzzyFilter(BaseVersionList::ModelRoles role, QString filter)
{
	m_proxyModel->setFilter(role, filter, false);
}

#include "VersionSelectDialog.moc"
