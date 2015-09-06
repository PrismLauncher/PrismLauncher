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

#include "WorldListPage.h"
#include "ui_WorldListPage.h"
#include "minecraft/WorldList.h"
#include "dialogs/ModEditDialogCommon.h"
#include <QEvent>
#include <QKeyEvent>
#include <QClipboard>
#include <QMessageBox>
#include <QTreeView>


#include "MultiMC.h"

WorldListPage::WorldListPage(BaseInstance *inst, std::shared_ptr<WorldList> worlds, QString id,
							 QString iconName, QString displayName, QString helpPage,
							 QWidget *parent)
	: QWidget(parent), ui(new Ui::WorldListPage), m_worlds(worlds), m_inst(inst), m_id(id), m_displayName(displayName), m_iconName(iconName), m_helpName(helpPage)
{
	ui->setupUi(this);
	ui->tabWidget->tabBar()->hide();
	QSortFilterProxyModel * proxy = new QSortFilterProxyModel(this);
	proxy->setSourceModel(m_worlds.get());
	ui->worldTreeView->setSortingEnabled(true);
	ui->worldTreeView->setModel(proxy);
	ui->worldTreeView->installEventFilter(this);

	auto head = ui->worldTreeView->header();

	head->setSectionResizeMode(0, QHeaderView::Stretch);
	head->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	connect(ui->worldTreeView->selectionModel(),
			SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)), this,
			SLOT(worldChanged(const QModelIndex &, const QModelIndex &)));
	worldChanged(QModelIndex(), QModelIndex());
}

void WorldListPage::opened()
{
	m_worlds->startWatching();
}

void WorldListPage::closed()
{
	m_worlds->stopWatching();
}

WorldListPage::~WorldListPage()
{
	m_worlds->stopWatching();
	delete ui;
}

bool WorldListPage::shouldDisplay() const
{
	if (m_inst)
		return !m_inst->isRunning();
	return true;
}

bool WorldListPage::worldListFilter(QKeyEvent *keyEvent)
{
	switch (keyEvent->key())
	{
	case Qt::Key_Delete:
		on_rmWorldBtn_clicked();
		return true;
	default:
		break;
	}
	return QWidget::eventFilter(ui->worldTreeView, keyEvent);
}

bool WorldListPage::eventFilter(QObject *obj, QEvent *ev)
{
	if (ev->type() != QEvent::KeyPress)
	{
		return QWidget::eventFilter(obj, ev);
	}
	QKeyEvent *keyEvent = static_cast<QKeyEvent *>(ev);
	if (obj == ui->worldTreeView)
		return worldListFilter(keyEvent);
	return QWidget::eventFilter(obj, ev);
}

void WorldListPage::on_rmWorldBtn_clicked()
{
	auto proxiedIndex = getSelectedWorld();

	if(!proxiedIndex.isValid())
		return;

	auto result = QMessageBox::question(this,
				tr("Are you sure?"),
				tr("This will remove the selected world permenantly.\n"
					"The world will be gone forever (A LONG TIME).\n"
					"\n"
					"Do you want to continue?"));
	if(result != QMessageBox::Yes)
	{
		return;
	}
	m_worlds->stopWatching();
	m_worlds->deleteWorld(proxiedIndex.row());
	m_worlds->startWatching();
}

void WorldListPage::on_viewFolderBtn_clicked()
{
	openDirInDefaultProgram(m_worlds->dir().absolutePath(), true);
}

QModelIndex WorldListPage::getSelectedWorld()
{
	auto index = ui->worldTreeView->selectionModel()->currentIndex();

	auto proxy = (QSortFilterProxyModel *) ui->worldTreeView->model();
	return proxy->mapToSource(index);
}

void WorldListPage::on_copySeedBtn_clicked()
{
	QModelIndex index = getSelectedWorld();

	if (!index.isValid())
	{
		return;
	}
	int64_t seed = m_worlds->data(index, WorldList::SeedRole).toLongLong();
	MMC->clipboard()->setText(QString::number(seed));
}

void WorldListPage::on_mcEditBtn_clicked()
{
	const QString mceditPath = MMC->settings()->get("MCEditPath").toString();

	QModelIndex index = getSelectedWorld();

	if (!index.isValid())
	{
		return;
	}

	auto fullPath = m_worlds->data(index, WorldList::FolderRole).toString();

#ifdef Q_OS_OSX
	QProcess *process = new QProcess();
	connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), process, SLOT(deleteLater()));
	process->setProgram(mceditPath);
	process->setArguments(QStringList() << fullPath);
	process->start();
#else
	QDir mceditDir(mceditPath);
	QString program;
	#ifdef Q_OS_LINUX
	if (mceditDir.exists("mcedit.py"))
	{
		program = mceditDir.absoluteFilePath("mcedit.py");
	}
	else if (mceditDir.exists("mcedit.sh"))
	{
		program = mceditDir.absoluteFilePath("mcedit.sh");
	}
	#elif defined(Q_OS_WIN32)
	if (mceditDir.exists("mcedit.exe"))
	{
		program = mceditDir.absoluteFilePath("mcedit.exe");
	}
	else if (mceditDir.exists("mcedit2.exe"))
	{
		program = mceditDir.absoluteFilePath("mcedit2.exe");
	}
	#endif
	if(program.size())
	{
		QProcess::startDetached(program, QStringList() << fullPath, mceditPath);
	}
#endif
}

void WorldListPage::worldChanged(const QModelIndex &current, const QModelIndex &previous)
{
	QModelIndex index = getSelectedWorld();
	bool enable = index.isValid();
	ui->copySeedBtn->setEnabled(enable);
	ui->mcEditBtn->setEnabled(enable);
	ui->rmWorldBtn->setEnabled(enable);
}
