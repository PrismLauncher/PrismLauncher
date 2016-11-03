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
#include <DesktopServices.h>
#include "dialogs/ModEditDialogCommon.h"
#include <QEvent>
#include <QKeyEvent>
#include <QClipboard>
#include <QMessageBox>
#include <QTreeView>
#include <QInputDialog>
#include <tools/MCEditTool.h>

#include "MultiMC.h"
#include <GuiUtil.h>
#include <QProcess>
#include <FileSystem.h>

WorldListPage::WorldListPage(BaseInstance *inst, std::shared_ptr<WorldList> worlds, QString id,
							 QString iconName, QString displayName, QString helpPage,
							 QWidget *parent)
	: QWidget(parent), m_inst(inst), ui(new Ui::WorldListPage), m_worlds(worlds), m_iconName(iconName), m_id(id), m_displayName(displayName), m_helpName(helpPage)
{
	ui->setupUi(this);
	ui->tabWidget->tabBar()->hide();
	QSortFilterProxyModel * proxy = new QSortFilterProxyModel(this);
	proxy->setSortCaseSensitivity(Qt::CaseInsensitive);
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
	DesktopServices::openDirectory(m_worlds->dir().absolutePath(), true);
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
	if(m_mceditStarting)
		return;

	auto mcedit = MMC->mcedit();

	const QString mceditPath = mcedit->path();

	QModelIndex index = getSelectedWorld();

	if (!index.isValid())
	{
		return;
	}

	if(!worldSafetyNagQuestion())
		return;

	auto fullPath = m_worlds->data(index, WorldList::FolderRole).toString();

	auto program = mcedit->getProgramPath();
	if(program.size())
	{
		m_mceditProcess.reset(new LoggedProcess());
		m_mceditProcess->setDetachable(true);
		connect(m_mceditProcess.get(), &LoggedProcess::stateChanged, this, &WorldListPage::mceditState);
		m_mceditProcess->start(program, {fullPath});
		m_mceditStarting = true;
	}
	else
	{
		QMessageBox::warning(
			this->parentWidget(),
			tr("No MCEdit found or set up!"),
			tr("You do not have MCEdit set up or it was moved.\nYou can set it up in the global settings.")
		);
	}
}

void WorldListPage::mceditState(LoggedProcess::State state)
{
	bool failed = false;
	switch(state)
	{
		case LoggedProcess::NotRunning:
		case LoggedProcess::Starting:
			return;
		case LoggedProcess::FailedToStart:
		case LoggedProcess::Crashed:
		case LoggedProcess::Aborted:
		{
			failed = true;
		}
		case LoggedProcess::Running:
		case LoggedProcess::Finished:
		{
			m_mceditStarting = false;
			break;
		}
	}
	if(failed)
	{
		QMessageBox::warning(
			this->parentWidget(),
			tr("MCEdit failed to start!"),
			tr("MCEdit failed to start.\nIt may be necessary to reinstall it.")
		);
	}
}

void WorldListPage::worldChanged(const QModelIndex &current, const QModelIndex &previous)
{
	QModelIndex index = getSelectedWorld();
	bool enable = index.isValid();
	ui->copySeedBtn->setEnabled(enable);
	ui->mcEditBtn->setEnabled(enable);
	ui->rmWorldBtn->setEnabled(enable);
	ui->copyBtn->setEnabled(enable);
	ui->renameBtn->setEnabled(enable);
}

void WorldListPage::on_addBtn_clicked()
{
	auto list = GuiUtil::BrowseForFiles(
		m_helpName,
		tr("Select a Minecraft world zip"),
		tr("Minecraft World Zip File (*.zip)"), QString(), this->parentWidget());
	if (!list.empty())
	{
		m_worlds->stopWatching();
		for (auto filename : list)
		{
			m_worlds->installWorld(QFileInfo(filename));
		}
		m_worlds->startWatching();
	}
}

bool WorldListPage::isWorldSafe(QModelIndex)
{
	return !m_inst->isRunning();
}

bool WorldListPage::worldSafetyNagQuestion()
{
	if(!isWorldSafe(getSelectedWorld()))
	{
		auto result = QMessageBox::question(this, tr("Copy World"), tr("Changing a world while Minecraft is running is potentially unsafe.\nDo you wish to proceed?"));
		if(result == QMessageBox::No)
		{
			return false;
		}
	}
	return true;
}


void WorldListPage::on_copyBtn_clicked()
{
	QModelIndex index = getSelectedWorld();
	if (!index.isValid())
	{
		return;
	}

	if(!worldSafetyNagQuestion())
		return;

	auto worldVariant = m_worlds->data(index, WorldList::ObjectRole);
	auto world = (World *) worldVariant.value<void *>();
	bool ok = false;
	QString name = QInputDialog::getText(this, tr("World name"), tr("Enter a new name for the copy."), QLineEdit::Normal, world->name(), &ok);

	if (ok && name.length() > 0)
	{
		world->install(m_worlds->dir().absolutePath(), name);
	}
}

void WorldListPage::on_renameBtn_clicked()
{
	QModelIndex index = getSelectedWorld();
	if (!index.isValid())
	{
		return;
	}

	if(!worldSafetyNagQuestion())
		return;

	auto worldVariant = m_worlds->data(index, WorldList::ObjectRole);
	auto world = (World *) worldVariant.value<void *>();

	bool ok = false;
	QString name = QInputDialog::getText(this, tr("World name"), tr("Enter a new world name."), QLineEdit::Normal, world->name(), &ok);

	if (ok && name.length() > 0)
	{
		world->rename(name);
	}
}

void WorldListPage::on_refreshBtn_clicked()
{
	m_worlds->update();
}
