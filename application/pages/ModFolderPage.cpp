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

#include "ModFolderPage.h"
#include "ui_ModFolderPage.h"

#include <QMessageBox>
#include <QEvent>
#include <QKeyEvent>
#include <QAbstractItemModel>

#include "MultiMC.h"
#include "dialogs/CustomMessageBox.h"
#include "dialogs/ModEditDialogCommon.h"
#include <GuiUtil.h>
#include "minecraft/ModList.h"
#include "minecraft/Mod.h"
#include "minecraft/VersionFilterData.h"
#include <DesktopServices.h>

ModFolderPage::ModFolderPage(BaseInstance *inst, std::shared_ptr<ModList> mods, QString id,
							 QString iconName, QString displayName, QString helpPage,
							 QWidget *parent)
	: QWidget(parent), ui(new Ui::ModFolderPage)
{
	ui->setupUi(this);
	ui->tabWidget->tabBar()->hide();
	m_inst = inst;
	m_mods = mods;
	m_id = id;
	m_displayName = displayName;
	m_iconName = iconName;
	m_helpName = helpPage;
	m_filter = "%1 (*.zip *.jar)";
	ui->modTreeView->setModel(m_mods.get());
	ui->modTreeView->installEventFilter(this);
	auto smodel = ui->modTreeView->selectionModel();
	connect(smodel, &QItemSelectionModel::currentChanged, this, &ModFolderPage::modCurrent);
}

void ModFolderPage::opened()
{
	m_mods->startWatching();
}

void ModFolderPage::closed()
{
	m_mods->stopWatching();
}

CoreModFolderPage::CoreModFolderPage(BaseInstance *inst, std::shared_ptr<ModList> mods,
									 QString id, QString iconName, QString displayName,
									 QString helpPage, QWidget *parent)
	: ModFolderPage(inst, mods, id, iconName, displayName, helpPage, parent)
{
}

ModFolderPage::~ModFolderPage()
{
	m_mods->stopWatching();
	delete ui;
}

bool ModFolderPage::shouldDisplay() const
{
	if (m_inst)
		return !m_inst->isRunning();
	return true;
}

bool CoreModFolderPage::shouldDisplay() const
{
	if (ModFolderPage::shouldDisplay())
	{
		auto inst = dynamic_cast<OneSixInstance *>(m_inst);
		if (!inst)
			return true;
		auto version = inst->getMinecraftProfile();
		if (!version)
			return true;
		if(!version->versionPatch("net.minecraftforge"))
		{
			return false;
		}
		if(!version->versionPatch("net.minecraft"))
		{
			return false;
		}
		if(version->versionPatch("net.minecraft")->getReleaseDateTime() < g_VersionFilterData.legacyCutoffDate)
		{
			return true;
		}
	}
	return false;
}

bool ModFolderPage::modListFilter(QKeyEvent *keyEvent)
{
	switch (keyEvent->key())
	{
	case Qt::Key_Delete:
		on_rmModBtn_clicked();
		return true;
	case Qt::Key_Plus:
		on_addModBtn_clicked();
		return true;
	default:
		break;
	}
	return QWidget::eventFilter(ui->modTreeView, keyEvent);
}

bool ModFolderPage::eventFilter(QObject *obj, QEvent *ev)
{
	if (ev->type() != QEvent::KeyPress)
	{
		return QWidget::eventFilter(obj, ev);
	}
	QKeyEvent *keyEvent = static_cast<QKeyEvent *>(ev);
	if (obj == ui->modTreeView)
		return modListFilter(keyEvent);
	return QWidget::eventFilter(obj, ev);
}

void ModFolderPage::on_addModBtn_clicked()
{
	auto list = GuiUtil::BrowseForFiles(
		m_helpName,
		tr("Select %1",
		   "Select whatever type of files the page contains. Example: 'Loader Mods'")
			.arg(m_displayName),
		m_filter.arg(m_displayName), MMC->settings()->get("CentralModsDir").toString(),
		this->parentWidget());
	if (!list.empty())
	{
		m_mods->stopWatching();
		for (auto filename : list)
		{
			m_mods->installMod(filename);
		}
		m_mods->startWatching();
	}
}

void ModFolderPage::on_rmModBtn_clicked()
{
	int first, last;
	auto list = ui->modTreeView->selectionModel()->selectedRows();

	if (!lastfirst(list, first, last))
		return;
	m_mods->stopWatching();
	m_mods->deleteMods(first, last);
	m_mods->startWatching();
}

void ModFolderPage::on_viewModBtn_clicked()
{
	DesktopServices::openDirectory(m_mods->dir().absolutePath(), true);
}

void ModFolderPage::modCurrent(const QModelIndex &current, const QModelIndex &previous)
{
	if (!current.isValid())
	{
		ui->frame->clear();
		return;
	}
	int row = current.row();
	Mod &m = m_mods->operator[](row);
	ui->frame->updateWithMod(m);
}
