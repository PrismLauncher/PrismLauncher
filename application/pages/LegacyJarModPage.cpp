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

#include "LegacyJarModPage.h"
#include "ui_LegacyJarModPage.h"

#include <QKeyEvent>
#include <QKeyEvent>

#include <pathutils.h>

#include "dialogs/VersionSelectDialog.h"
#include "dialogs/ProgressDialog.h"
#include "dialogs/ModEditDialogCommon.h"
#include "minecraft/ModList.h"
#include "minecraft/LegacyInstance.h"
#include "Env.h"
#include "MultiMC.h"
#include <GuiUtil.h>

LegacyJarModPage::LegacyJarModPage(LegacyInstance *inst, QWidget *parent)
	: QWidget(parent), ui(new Ui::LegacyJarModPage), m_inst(inst)
{
	ui->setupUi(this);
	ui->tabWidget->tabBar()->hide();

	m_jarmods = m_inst->jarModList();
	ui->jarModsTreeView->setModel(m_jarmods.get());
	ui->jarModsTreeView->setDragDropMode(QAbstractItemView::DragDrop);
	ui->jarModsTreeView->setSelectionMode(QAbstractItemView::SingleSelection);
	ui->jarModsTreeView->installEventFilter(this);
	m_jarmods->startWatching();
	auto smodel = ui->jarModsTreeView->selectionModel();
	connect(smodel, SIGNAL(currentChanged(QModelIndex, QModelIndex)),
			SLOT(jarCurrent(QModelIndex, QModelIndex)));
}

LegacyJarModPage::~LegacyJarModPage()
{
	m_jarmods->stopWatching();
	delete ui;
}

bool LegacyJarModPage::shouldDisplay() const
{
	return !m_inst->isRunning();
}

bool LegacyJarModPage::eventFilter(QObject *obj, QEvent *ev)
{
	if (ev->type() != QEvent::KeyPress || obj != ui->jarModsTreeView)
	{
		return QWidget::eventFilter(obj, ev);
	}

	QKeyEvent *keyEvent = static_cast<QKeyEvent *>(ev);
	switch (keyEvent->key())
	{
	case Qt::Key_Up:
	{
		if (keyEvent->modifiers() & Qt::ControlModifier)
		{
			on_moveJarUpBtn_clicked();
			return true;
		}
		break;
	}
	case Qt::Key_Down:
	{
		if (keyEvent->modifiers() & Qt::ControlModifier)
		{
			on_moveJarDownBtn_clicked();
			return true;
		}
		break;
	}
	case Qt::Key_Delete:
		on_rmJarBtn_clicked();
		return true;
	case Qt::Key_Plus:
		on_addJarBtn_clicked();
		return true;
	default:
		break;
	}
	return QWidget::eventFilter(obj, ev);
}

void LegacyJarModPage::on_addJarBtn_clicked()
{
	auto list = GuiUtil::BrowseForMods("jarmod", tr("Select jar mods"), tr("Minecraft.jar mods (*.zip *.jar)"), this->parentWidget());
	if(!list.empty())
	{
		m_jarmods->stopWatching();
		for (auto filename : list)
		{
			m_jarmods->installMod(QFileInfo(filename));
		}
		m_jarmods->startWatching();
	}
}

void LegacyJarModPage::on_moveJarDownBtn_clicked()
{
	int first, last;
	auto list = ui->jarModsTreeView->selectionModel()->selectedRows();

	if (!lastfirst(list, first, last))
		return;

	m_jarmods->moveModsDown(first, last);
}

void LegacyJarModPage::on_moveJarUpBtn_clicked()
{
	int first, last;
	auto list = ui->jarModsTreeView->selectionModel()->selectedRows();

	if (!lastfirst(list, first, last))
		return;
	m_jarmods->moveModsUp(first, last);
}

void LegacyJarModPage::on_rmJarBtn_clicked()
{
	int first, last;
	auto list = ui->jarModsTreeView->selectionModel()->selectedRows();

	if (!lastfirst(list, first, last))
		return;
	m_jarmods->stopWatching();
	m_jarmods->deleteMods(first, last);
	m_jarmods->startWatching();
}

void LegacyJarModPage::on_viewJarBtn_clicked()
{
	openDirInDefaultProgram(m_inst->jarModsDir(), true);
}

void LegacyJarModPage::jarCurrent(QModelIndex current, QModelIndex previous)
{
	if (!current.isValid())
	{
		ui->jarMIFrame->clear();
		return;
	}
	int row = current.row();
	Mod &m = m_jarmods->operator[](row);
	ui->jarMIFrame->updateWithMod(m);
}
