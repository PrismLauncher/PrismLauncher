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

#include "MultiMC.h"

#include <pathutils.h>
#include <QFileDialog>
#include <QMessageBox>
#include <QEvent>
#include <QKeyEvent>

#include "VersionPage.h"
#include "ui_VersionPage.h"

#include "Platform.h"
#include "dialogs/CustomMessageBox.h"
#include "dialogs/VersionSelectDialog.h"
#include "dialogs/ModEditDialogCommon.h"

#include "dialogs/ProgressDialog.h"
#include <GuiUtil.h>

#include <QAbstractItemModel>
#include <QMessageBox>
#include <QListView>
#include <QString>
#include <QUrl>

#include "minecraft/MinecraftProfile.h"
#include "forge/ForgeVersionList.h"
#include "forge/ForgeInstaller.h"
#include "liteloader/LiteLoaderVersionList.h"
#include "liteloader/LiteLoaderInstaller.h"
#include "auth/MojangAccountList.h"
#include "minecraft/Mod.h"
#include "icons/IconList.h"


QIcon VersionPage::icon() const
{
	return ENV.icons()->getIcon(m_inst->iconKey());
}
bool VersionPage::shouldDisplay() const
{
	return !m_inst->isRunning();
}

VersionPage::VersionPage(OneSixInstance *inst, QWidget *parent)
	: QWidget(parent), ui(new Ui::VersionPage), m_inst(inst)
{
	ui->setupUi(this);
	ui->tabWidget->tabBar()->hide();

	reloadMinecraftProfile();

	m_version = m_inst->getMinecraftProfile();
	if (m_version)
	{
		ui->libraryTreeView->setModel(m_version.get());
		ui->libraryTreeView->installEventFilter(this);
		ui->libraryTreeView->setSelectionMode(QAbstractItemView::SingleSelection);
		connect(ui->libraryTreeView->selectionModel(), &QItemSelectionModel::currentChanged,
				this, &VersionPage::versionCurrent);
		updateVersionControls();
		// select first item.
		auto index = ui->libraryTreeView->model()->index(0,0);
		if(index.isValid())
			ui->libraryTreeView->setCurrentIndex(index);
	}
	else
	{
		disableVersionControls();
	}
	connect(m_inst, &OneSixInstance::versionReloaded, this,
			&VersionPage::updateVersionControls);
}

VersionPage::~VersionPage()
{
	delete ui;
}

void VersionPage::updateVersionControls()
{
	ui->forgeBtn->setEnabled(true);
	ui->liteloaderBtn->setEnabled(true);
}

void VersionPage::disableVersionControls()
{
	ui->forgeBtn->setEnabled(false);
	ui->liteloaderBtn->setEnabled(false);
	ui->reloadLibrariesBtn->setEnabled(false);
	ui->removeLibraryBtn->setEnabled(false);
}

bool VersionPage::reloadMinecraftProfile()
{
	try
	{
		m_inst->reloadProfile();
		return true;
	}
	catch (MMCError &e)
	{
		QMessageBox::critical(this, tr("Error"), e.cause());
		return false;
	}
	catch (...)
	{
		QMessageBox::critical(
			this, tr("Error"),
			tr("Couldn't load the instance profile."));
		return false;
	}
}

void VersionPage::on_reloadLibrariesBtn_clicked()
{
	reloadMinecraftProfile();
}

void VersionPage::on_removeLibraryBtn_clicked()
{
	if (ui->libraryTreeView->currentIndex().isValid())
	{
		// FIXME: use actual model, not reloading.
		if (!m_version->remove(ui->libraryTreeView->currentIndex().row()))
		{
			QMessageBox::critical(this, tr("Error"), tr("Couldn't remove file"));
		}
	}
}

void VersionPage::on_jarmodBtn_clicked()
{
	auto list = GuiUtil::BrowseForMods("jarmod", tr("Select jar mods"), tr("Minecraft.jar mods (*.zip *.jar)"), this->parentWidget());
	if(!list.empty())
	{
		m_version->installJarMods(list);
	}
}

void VersionPage::on_resetLibraryOrderBtn_clicked()
{
	try
	{
		m_version->resetOrder();
	}
	catch (MMCError &e)
	{
		QMessageBox::critical(this, tr("Error"), e.cause());
	}
}

void VersionPage::on_moveLibraryUpBtn_clicked()
{
	if (ui->libraryTreeView->selectionModel()->selectedRows().isEmpty())
	{
		return;
	}
	try
	{
		const int row = ui->libraryTreeView->selectionModel()->selectedRows().first().row();
		m_version->move(row, MinecraftProfile::MoveUp);
	}
	catch (MMCError &e)
	{
		QMessageBox::critical(this, tr("Error"), e.cause());
	}
}

void VersionPage::on_moveLibraryDownBtn_clicked()
{
	if (ui->libraryTreeView->selectionModel()->selectedRows().isEmpty())
	{
		return;
	}
	try
	{
		const int row = ui->libraryTreeView->selectionModel()->selectedRows().first().row();
		m_version->move(row, MinecraftProfile::MoveDown);
	}
	catch (MMCError &e)
	{
		QMessageBox::critical(this, tr("Error"), e.cause());
	}
}

void VersionPage::on_changeMCVersionBtn_clicked()
{
	VersionSelectDialog vselect(m_inst->versionList().get(), tr("Change Minecraft version"),
								this);
	if (!vselect.exec() || !vselect.selectedVersion())
		return;

	if (!MMC->accounts()->anyAccountIsValid())
	{
		CustomMessageBox::selectable(
			this, tr("Error"),
			tr("MultiMC cannot download Minecraft or update instances unless you have at least "
			   "one account added.\nPlease add your Mojang or Minecraft account."),
			QMessageBox::Warning)->show();
		return;
	}

	if (!m_version->isVanilla())
	{
		auto result = CustomMessageBox::selectable(
			this, tr("Are you sure?"),
			tr("This will remove any library/version customization you did previously. "
			   "This includes things like Forge install and similar."),
			QMessageBox::Warning, QMessageBox::Ok | QMessageBox::Abort,
			QMessageBox::Abort)->exec();

		if (result != QMessageBox::Ok)
			return;
		m_version->revertToVanilla();
		reloadMinecraftProfile();
	}
	m_inst->setIntendedVersionId(vselect.selectedVersion()->descriptor());

	auto updateTask = m_inst->doUpdate();
	if (!updateTask)
	{
		return;
	}
	ProgressDialog tDialog(this);
	connect(updateTask.get(), SIGNAL(failed(QString)), SLOT(onGameUpdateError(QString)));
	tDialog.exec(updateTask.get());
}

void VersionPage::on_forgeBtn_clicked()
{
	VersionSelectDialog vselect(MMC->forgelist().get(), tr("Select Forge version"), this);
	vselect.setExactFilter(BaseVersionList::ParentGameVersionRole, m_inst->currentVersionId());
	vselect.setEmptyString(tr("No Forge versions are currently available for Minecraft ") +
						   m_inst->currentVersionId());
	vselect.setEmptyErrorString(tr("Couldn't load or download the Forge version lists!"));
	if (vselect.exec() && vselect.selectedVersion())
	{
		ProgressDialog dialog(this);
		dialog.exec(
			ForgeInstaller().createInstallTask(m_inst, vselect.selectedVersion(), this));
	}
}

void VersionPage::on_liteloaderBtn_clicked()
{
	VersionSelectDialog vselect(MMC->liteloaderlist().get(), tr("Select LiteLoader version"),
								this);
	vselect.setExactFilter(BaseVersionList::ParentGameVersionRole, m_inst->currentVersionId());
	vselect.setEmptyString(tr("No LiteLoader versions are currently available for Minecraft ") +
						   m_inst->currentVersionId());
	vselect.setEmptyErrorString(tr("Couldn't load or download the LiteLoader version lists!"));
	if (vselect.exec() && vselect.selectedVersion())
	{
		ProgressDialog dialog(this);
		dialog.exec(
			LiteLoaderInstaller().createInstallTask(m_inst, vselect.selectedVersion(), this));
	}
}

void VersionPage::versionCurrent(const QModelIndex &current, const QModelIndex &previous)
{
	if (!current.isValid())
	{
		ui->removeLibraryBtn->setDisabled(true);
		ui->moveLibraryDownBtn->setDisabled(true);
		ui->moveLibraryUpBtn->setDisabled(true);
	}
	else
	{
		bool enabled = m_version->canRemove(current.row());
		ui->removeLibraryBtn->setEnabled(enabled);
		ui->moveLibraryDownBtn->setEnabled(enabled);
		ui->moveLibraryUpBtn->setEnabled(enabled);
	}
	QString selectedId = m_version->versionFileId(current.row());
	if (selectedId == "net.minecraft")
	{
		ui->changeMCVersionBtn->setEnabled(true);
	}
	else
	{
		ui->changeMCVersionBtn->setEnabled(false);
	}
}

void VersionPage::onGameUpdateError(QString error)
{
	CustomMessageBox::selectable(this, tr("Error updating instance"), error,
								 QMessageBox::Warning)->show();
}
