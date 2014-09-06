/* Copyright 2013 MultiMC Contributors
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

#include <gui/dialogs/ProgressDialog.h>
#include "gui/Platform.h"

#include <logic/BaseVersion.h>
#include <logic/BaseVersionList.h>
#include <logic/tasks/Task.h>
#include <depends/util/include/modutils.h>
#include "logger/QsLog.h"

class VersionSelectProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT
public:
	VersionSelectProxyModel(QObject *parent = 0) : QSortFilterProxyModel(parent)
	{
	}

	struct Filter
	{
		QString string;
		bool exact = false;
	};

	QHash<int, Filter> filters() const
	{
		return m_filters;
	}
	void setFilter(const int column, const QString &filter, const bool exact)
	{
		Filter f;
		f.string = filter;
		f.exact = exact;
		m_filters[column] = f;
		invalidateFilter();
	}
	void clearFilters()
	{
		m_filters.clear();
		invalidateFilter();
	}

protected:
	bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
	{
		for (auto it = m_filters.begin(); it != m_filters.end(); ++it)
		{
			const QString version =
				sourceModel()->index(source_row, it.key()).data().toString();

			if (it.value().exact)
			{
				if (version != it.value().string)
				{
					return false;
				}
				continue;
			}

			if (!Util::versionIsInInterval(version, it.value().string))
			{
				return false;
			}
		}
		return true;
	}

	QHash<int, Filter> m_filters;
};

VersionSelectDialog::VersionSelectDialog(BaseVersionList *vlist, QString title, QWidget *parent,
										 bool cancelable)
	: QDialog(parent), ui(new Ui::VersionSelectDialog), m_useLatest(false)
{
	MultiMCPlatform::fixWM_CLASS(this);
	ui->setupUi(this);
	setWindowModality(Qt::WindowModal);
	setWindowTitle(title);

	m_vlist = vlist;

	m_proxyModel = new VersionSelectProxyModel(this);
	m_proxyModel->setSourceModel(vlist);

	ui->listView->setModel(m_proxyModel);
	ui->listView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
	ui->listView->header()->setSectionResizeMode(resizeOnColumn, QHeaderView::Stretch);

	if (!cancelable)
	{
		ui->buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(false);
	}
}

void VersionSelectDialog::setEmptyString(QString emptyString)
{
	ui->listView->setEmptyString(emptyString);
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

void VersionSelectDialog::setUseLatest(const bool useLatest)
{
	m_useLatest = useLatest;
}

int VersionSelectDialog::exec()
{
	QDialog::open();
	if (!m_vlist->isLoaded())
	{
		loadList();
	}
	m_proxyModel->invalidate();
	if (m_proxyModel->rowCount() == 0)
	{
		QLOG_DEBUG() << "No rows in version list";
		return QDialog::Rejected;
	}
	if (m_proxyModel->rowCount() == 1 || m_useLatest)
	{
		ui->listView->selectionModel()->setCurrentIndex(m_proxyModel->index(0, 0),
														QItemSelectionModel::ClearAndSelect);
		return QDialog::Accepted;
	}
	return QDialog::exec();
}

void VersionSelectDialog::loadList()
{
	Task *loadTask = m_vlist->getLoadTask();
	if (!loadTask)
	{
		return;
	}
	ProgressDialog *taskDlg = new ProgressDialog(this);
	loadTask->setParent(taskDlg);
	taskDlg->exec(loadTask);
	delete taskDlg;
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

void VersionSelectDialog::setExactFilter(int column, QString filter)
{
	m_proxyModel->setFilter(column, filter, true);
}

void VersionSelectDialog::setFuzzyFilter(int column, QString filter)
{
	m_proxyModel->setFilter(column, filter, false);
}

#include "VersionSelectDialog.moc"
